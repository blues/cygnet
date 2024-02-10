// Copyright 2017 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "app.h"
#include "global.h"

// Is location set/valid
STATIC bool locationValid = false;
STATIC double locationLat = 0;
STATIC double locationLon = 0;
STATIC uint32_t locationSecs = 0;

// Set the location if it's better than what we've got
bool locSetIfBetter(double lat, double lon, uint32_t ltime)
{
    if (!timeIsValidUnix(ltime)) {
        ltime = 0;
    }
    if (!locValid() || !timeIsValidUnix(locationSecs)) {
        return locSet(lat, lon, ltime);
    }
    if (ltime >= locationSecs) {
        return locSet(lat, lon, ltime);
    }
    return false;
}

// Set the location
bool locSet(double lat, double lon, uint32_t ltime)
{
    locationLat = lat;
    locationLon = lon;
    locationSecs = ltime;
    // Although lat/lon of 0 is valid, it's not realistic and it's
    // more frequently indicative of a "not set" condition
    if (lat == 0 && lon == 0) {
        locationValid = false;
    } else {
        locationValid = true;
    }
    return true;
}

// Get the location
bool locGet(double *lat, double *lon, uint32_t *when)
{
    if (!locationValid) {
        if (lat != NULL) {
            *lat = 0.0;
        }
        if (lon != NULL) {
            *lon = 0.0;
        }
        if (when != NULL) {
            *when = 0;
        }
        return false;
    }
    if (lat != NULL) {
        *lat = locationLat;
    }
    if (lon != NULL) {
        *lon = locationLon;
    }
    if (when != NULL) {
        *when = locationSecs;
    }
    return true;
}

// See if the location is valid
bool locValid(void)
{
    return locationValid;
}

// Invalidate the current location
void locInvalidate(void)
{
    locationValid = false;
}
