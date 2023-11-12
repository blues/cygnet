
#include <string.h>
#include "app.h"

// For time conversion
#include <time.h>
extern time_t rk_timegm (struct tm *tm);

// Globals
double gpsLastHDOP = 0;
double gpsLastLat = 0;
double gpsLastLon = 0;
int64_t gpsLastLatLonMs = 0;
uint32_t gpsLastLatLonSecs = 0;
uint32_t gpsLastTimeSecs = 0;

// Forwards
double gpsEncodingToDegrees(char *inlocation, char *inzone);

// Process a line received by the GPS, and return true if this should be passed to the notecard
// http://www.gpsinformation.org/dale/nmea.htm
bool gpsReceivedLine(char *linebuf)
{
    char line[256];
    strLcpy(line, linebuf);
    uint32_t linelen = strlen(line);
    bool processedLine = false;

    // Discard anything that doesn't look like a GPS sentence.
    if (line[0] != '$' || line[1] != 'G') {
        return false;
    }

    // Compute the checksum, and discard lines whose checksum isn't valid
    char *xp = &line[1];
    uint16_t xlen = linelen-1;
    char *asterisk = memchr(xp, '*', xlen);
    if (asterisk == NULL) {
        return false;
    }
    xlen = (uint16_t) (asterisk - xp);
    uint8_t xsum = 0;
    for (int i=0; i<xlen; i++) {
        xsum ^= xp[i];
    }
    char xstr[10];
    snprintf(xstr, sizeof(xstr), "%02x", xsum);
    if (!memeqlCI(asterisk+1, xstr, 2)) {
        return false;
    }

    // Process $GxGGA, which should give us lat/lon/alt
    if (memcmp(&line[3], "GGA", 3) == 0) {
        processedLine = true;

        int j;
        char *lat, *ns, *lon, *ew, *alt, *fix, *time, *nsat, *hdop;
        bool haveLat, haveLon, haveNS, haveEW, haveAlt, haveFix, haveTime, haveNSat, haveHdop;
        uint16_t commas;

        commas = 0;
        haveLat = haveLon = haveNS = haveEW = haveAlt = haveFix = haveTime = haveNSat = haveHdop = false;
        lat = ns = lon = ew = alt = time = fix = nsat = hdop = "";

        UNUSED_VARIABLE(haveAlt);
        UNUSED_VARIABLE(haveTime);
        UNUSED_VARIABLE(haveNSat);

        for (j = 0; j < linelen; j++) {
            if (line[j] == ',') {
                line[j] = '\0';
                commas++;
                if (commas == 1) {
                    // Sitting at comma before Time
                    time = (char *) &line[j + 1];
                }
                if (commas == 2) {
                    haveTime = (time[0] != '\0');
                    // Sitting at comma before Lat
                    lat = (char *) &line[j + 1];
                }
                if (commas == 3) {
                    // Sitting at comma before NS
                    haveLat = (lat[0] != '\0');
                    ns = (char *) &line[j + 1];
                }
                if (commas == 4) {
                    // Sitting at comma before Lon
                    haveNS = (ns[0] != '\0');
                    lon = (char *) &line[j + 1];
                }
                if (commas == 5) {
                    // Sitting at comma before EW
                    haveLon = (lon[0] != '\0');
                    ew = (char *) &line[j + 1];
                }
                if (commas == 6) {
                    // Sitting at comma before Quality
                    haveEW = (ew[0] != '\0');
                    fix = (char *) &line[j + 1];
                }
                if (commas == 7) {
                    // Sitting at comma before NumSat
                    // 1 is valid GPS fix, 2 is valid DGPS fix, 3 is valid PPS fix
                    haveFix = (fix[0] == '1' || fix[0] == '2' || fix[0] == '3');
                    nsat = (char *) &line[j + 1];
                }
                if (commas == 8) {
                    haveNSat = (nsat[0] != '\0');
                    // Sitting at comma before Hdop
                    hdop = (char *) &line[j + 1];
                }
                if (commas == 9) {
                    haveHdop = (hdop[0] != '\0');
                    // Sitting at comma before Alt
                    alt = (char *) &line[j + 1];
                }
                if (commas == 10) {
                    // Sitting at comma before M unit
                    haveAlt = (alt[0] != '\0');
                    break;
                }
            }
        }

        // If we've got what we need, process it and exit.
        if (haveFix && haveLat && haveNS & haveLon && haveEW && haveHdop) {
            gpsLastHDOP = strtod(hdop, NULL);
            if (gpsLastHDOP == 0) {
                gpsLastHDOP++;
            }
            gpsLastLat = gpsEncodingToDegrees(lat, ns);
            gpsLastLon = gpsEncodingToDegrees(lon, ew);
            gpsLastLatLonMs = timerMs();
            gpsLastTimeSecs = timeSecs();
            locSetIfBetter(gpsLastLat, gpsLastLon, gpsLastTimeSecs);
        }

    }   // if GxGGA

    // Process $GxRMC, which should give us lat/lon and time
    if (memcmp(&line[3], "RMC", 3) == 0) {
        processedLine = true;

        int j;
        char *lat, *ns, *lon, *ew, *time, *date, *valid;
        bool haveLat, haveLon, haveNS, haveEW, haveTime, haveDate, haveValid;
        uint16_t commas;

        commas = 0;
        haveLat = haveLon = haveNS = haveEW = haveTime = haveDate = haveValid = false;
        lat = ns = lon = ew = date = time = valid = "";

        UNUSED_VARIABLE(haveValid);

        for (j = 0; j < linelen; j++) {
            if (line[j] == ',') {
                line[j] = '\0';
                commas++;
                if (commas == 1) {
                    // Sitting at comma before Time
                    time = (char *) &line[j + 1];
                }
                if (commas == 2) {
                    haveTime = (time[0] != '\0');
                    // Sitting at comma before Validity
                    valid = (char *) &line[j + 1];
                }
                if (commas == 3) {
                    // Sitting at comma before Lat
                    haveValid = (valid[0] == 'A');
                    lat = (char *) &line[j + 1];
                }
                if (commas == 4) {
                    // Sitting at comma before NS
                    haveLat = (lat[0] != '\0');
                    ns = (char *) &line[j + 1];
                }
                if (commas == 5) {
                    // Sitting at comma before Lon
                    haveNS = (ns[0] != '\0');
                    lon = (char *) &line[j + 1];
                }
                if (commas == 6) {
                    // Sitting at comma before EW
                    haveLon = (lon[0] != '\0');
                    ew = (char *) &line[j + 1];
                }
                if (commas == 7) {
                    // Sitting at comma before Speed
                    haveEW = (ew[0] != '\0');
                }
                if (commas == 8) {
                    // Sitting at comma before True Course
                }
                if (commas == 9) {
                    // Sitting at comma before Date
                    date = (char *) &line[j + 1];
                }
                if (commas == 10) {
                    // Sitting at comma before Variation
                    haveDate = (date[0] != '\0');
                    break;
                }
            }
        }

        // Extract date/time
        if (haveTime && haveDate) {
            // http://aprs.gids.nl/nmea/#rmc
            // 225446 Time of fix 22:54:46 UTC
            // 191194 Date of fix  19 November 1994
            uint32_t ddmmyy = atol(date);
            uint32_t hhmmss = atol(time);
            uint16_t yr = (ddmmyy % 100) + 2000;
            uint16_t mo = (ddmmyy/100) % 100;
            uint16_t dy = (ddmmyy/10000) % 100;
            uint16_t ss = hhmmss % 100;
            uint16_t mm = (hhmmss/100) % 100;
            uint16_t hh = (hhmmss/10000) % 100;
            struct tm t = {0};
            t.tm_year = yr-1900;
            t.tm_mon = mo-1;
            t.tm_mday = dy;
            t.tm_hour = hh;
            t.tm_min = mm;
            t.tm_sec = ss;
            time_t epoch = rk_timegm(&t);
            if (timeIsValidUnix(epoch)) {
                gpsLastTimeSecs = epoch;
            }
            timeSetIfBetter(gpsLastTimeSecs);
        }

        // If we've got what we need, process it and exit.
        if (haveLat && haveNS & haveLon && haveEW) {
            gpsLastLat = gpsEncodingToDegrees(lat, ns);
            gpsLastLon = gpsEncodingToDegrees(lon, ew);
            gpsLastLatLonMs = timerMs();
            gpsLastTimeSecs = timeSecs();
            locSetIfBetter(gpsLastLat, gpsLastLon, gpsLastTimeSecs);
        }


    }   // if GxMRC

    // Process GSV, which might be interesting because of nsat and SNR
    if (memcmp(&line[3], "GSV", 3) == 0) {
        processedLine = true;
    }   // if GxGSV

    // Done
    return processedLine;

}

// Utility function to convert GPS DDDMM.MMMM N/S strings to signed degrees
double gpsEncodingToDegrees(char *inlocation, char *inzone)
{
    double a, r;
    int i, d;
    a = strtod(inlocation, NULL);
    i = (int) a;
    d = i / 100;
    a = a - (((double)d) * 100);
    r = ((double) d) + (a / 60);
    if (inzone[0] == 'S' || inzone[0] == 's' || inzone[0] == 'W' || inzone[0] == 'w') {
        r = -r;
    }
    return (r);
}
