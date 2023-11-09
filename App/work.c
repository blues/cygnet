
#include "app.h"

// Utility method to count commas in a string
int countCommaSeparated(char *p)
{

    // 0 if empty string
    if (p == NULL || p[0] == '\0') {
        return 0;
    }

    // Return number of commas plus one
    int count = 1;
    while (true) {
        char *s = strchr(p, ',');
        if (s == NULL) {
            break;
        }
        count++;
        p = s+1;
    }
    return count;

}

// Connect that the modem be powered on
err_t workModemConnect(J *body, uint8_t *payload, uint32_t payloadLen)
{
    err_t err;
    char cmd[48];

    // Exit if already connected
    if (modemIsConnected()) {
        return errF("already connected");
    }

    // The location will be needed below
    double lat, lon;
    if (!locGet(&lat, &lon, NULL)) {
        return errF("location not available");
    }

    // Power-on the modem, and then power it off, to get the parameters
    strLcpy(workDetailedStatus, "powering up");
    modemReportStatus();
    err = modemPowerOn();
    if (err) {
        return err;
    }
    strLcpy(workDetailedStatus, "initializing modem");
    modemReportStatus();

    // Enable network registration and location information via URC in this format:
    // +CEREG: <stat>
    // <stat> Integer type. EPS registration status.
    //    0 Not registered, not currently searching an operator to register to
    //    1 Registered on home network
    //    2 Not registered, but currently trying to attach or register
    //    3 Registration denied
    //    4 Unknown (e.g. out of coverage)
    //    5 Registered on roaming network
    err = modemSend(NULL, "AT+CEREG=1");
    if (err) {
        modemPowerOff();
        return err;
    }

    // Enable connection status information via URC
    // +CSCON: <mode>
    //    <mode> Integer type. The signaling connection status.
    //    0 Idle
    //    1 Connected
    err = modemSend(NULL, "AT+CSCON=1");
    if (err) {
        modemPowerOff();
        return err;
    }

    // Enable downlink URC reporting
    // +CRTDCP: <cid>,<cpdata_lenth>,<cpdata>;
    err = modemSend(NULL, "AT+CRTDCP=1");
    if (err) {
        modemPowerOff();
        return err;
    }

    // Pause while radio is enabled
    timerMsSleep(2500);

    // Clear qflock
    err = modemSend(NULL, "AT+QLOCKF=0", cmd);
    if (err) {
        modemPowerOff();
        return err;
    }

    // Lock to the specific NB-IoT frequency and channel
    int args = countCommaSeparated(configChannel);
    if (args != 0) {
        if (args == 1) {
            // 3 means offset is 0 according to AT docs
            snprintf(cmd, sizeof(cmd), "1,%s,3", configChannel);
        } else {
            strLcpy(cmd, "1,");
            strLcat(cmd, configChannel);
        }
        err = modemSend(NULL, "AT+QLOCKF=%s", cmd);
        if (err) {
            modemPowerOff();
            return err;
        }
    }

    // Set to NIDD and set our APN
    err = modemSend(NULL, "AT+QCGDEFCONT=\"Non-IP\",\"%s\"", configApn);
    if (err) {
        modemPowerOff();
        return err;
    }

    // Set bands
    int count = countCommaSeparated(configBand);
    if (count == 0) {
        strLcpy(cmd, "0");
    } else {
        snprintf(cmd, sizeof(cmd), "%d,%s", count, configBand);
    }
    err = modemSend(NULL, "AT+QBAND=%s", cmd);
    if (err) {
        modemPowerOff();
        return err;
    }

    // Validate the APN for trace purposes
    err = modemSend(NULL, "AT+QCGDEFCONT?");
    if (err) {
        modemPowerOff();
        return err;
    }

    // Enable radio.
    err = modemSend(NULL, "AT+CFUN=1");
    if (err) {
        modemPowerOff();
        return err;
    }

    // Network status (generally fails)
#if 0
    err = modemSend(NULL, "AT*MENGINFO=1");
    if (err) {
        modemPowerOff();
        return err;
    }
#endif

    // Make sure that the module knows our location
    err = modemSend(NULL, "AT+QGNSSINFO=%f,%f,0,0,0", lat, lon);
    if (err) {
        modemPowerOff();
        return err;
    }

    // Enable network registration and location information via URC in this format:
    // +CEREG: <stat>
    // <stat> Integer type. EPS registration status.
    //    0 Not registered, not currently searching an operator to register to
    //    1 Registered on home network
    //    2 Not registered, but currently trying to attach or register
    //    3 Registration denied
    //    4 Unknown (e.g. out of coverage)
    //    5 Registered on roaming network
    for (int i=0;; i++) {
        err = modemSend(NULL, "AT+CPIN?");
        if (!err) {
            if (!modemUrc("+CPIN: READY", false)) {
                err = errF("SIM not ready");
            }
        }
        if (!err) {
            break;
        }
        if (i >= 10) {
            modemPowerOff();
            return errF("SIM not ready");
        }
        timerMsSleep(750);
        modemProcessSerialIncoming();
    }

    // Wait for the registration to complete
    bool registered = false;
    int maxSecs = 120;
    int64_t beganMs = timerMs();
    int64_t expiresMs = beganMs + ((int64_t)maxSecs * ms1Sec);
    int messageSecs = 0;
    while (timerMs() < expiresMs) {
        if (modemUrc("+CEREG: 1", false) || modemUrc("+CEREG: 5", false) || modemUrc("+CEREG: 1,1", false) || modemUrc("+CEREG: 1,5", false)) {
            registered = true;
            break;
        }
        int elapsedSecs = (int) (timerMs() - beganMs) / 1000;
        if (elapsedSecs/5 != messageSecs/5) {
            messageSecs = elapsedSecs;
            snprintf(cmd, sizeof(cmd), "waiting for satellite network (%d/%d secs)", elapsedSecs, maxSecs);
            strLcpy(workDetailedStatus, cmd);
            modemReportStatus();
        }
        timerMsSleep(1000);
        modemSend(NULL, "AT+QENG=0");
        modemSend(NULL, "AT+CEREG?");
        modemProcessSerialIncoming();
    }
    if (!registered) {
        modemPowerOff();
        return errF("can't register with network");
    }

    // We're now connected
    modemSetConnected(true);

    // Done
    return errNone;

}

// Request that the modem be powered off
err_t workModemDisconnect(J *body, uint8_t *payload, uint32_t payloadLen)
{

    // Exit if already connected
    if (!modemIsOn()) {
        return errF("already disconnected");
    }

    // Disconnect from the network
    err_t err = modemSend(NULL, "AT+CFUN=0");
    if (err) {
        timerMsSleep(2500);
    }
    int maxSecs = 20;
    int64_t unregExpiresMs = timerMs() + ((int64_t)maxSecs * ms1Sec);
    while (timerMs() < unregExpiresMs) {
        if (modemUrc("+CEREG: 0", false) || modemUrc("+CEREG: 1,0", false) || modemUrc("+CEREG: 0,0", false)) {
            break;
        }
        modemSend(NULL, "AT+CEREG?");
        timerMsSleep(1000);
        modemProcessSerialIncoming();
    }

    // Do a power-down of the modem
#ifndef MODEM_ALWAYS_ON
    modemSend(NULL, "AT+QPOWD=0");
#endif

    // Power off
    modemPowerOff();

    return errNone;

}

// Request to uplink data
err_t workModemUplink(J *body, uint8_t *payload, uint32_t payloadLen)
{

    // Exit if nothing to do
    if (payloadLen == 0) {
        return errNone;
    }

    // Exit if the payload violates policy
    if (payloadLen > configMtu) {
        return errF("packet length of %d is larger than allowed MTU of %d", payloadLen, configMtu);
    }

    // Prefix and suffix for hex string
    char prefix[40];
    snprintf(prefix, sizeof(prefix), "AT+CSODCP=1,%d,\"", payloadLen);
    uint32_t prefixLen = strlen(prefix);
    char *suffix = "\"";
    uint32_t suffixLen = strlen(suffix);

    // Allocate the hex buffer
    char *cmd;
    err_t err = memAlloc(prefixLen + (payloadLen*2) + suffixLen + 1, &cmd);
    if (err) {
        return err;
    }
    char *p = cmd;
    memcpy(p, prefix, prefixLen);
    p += prefixLen;
    for (int i=0; i<payloadLen; i++) {
        htoa8(payload[i], (uint8_t *)p);
        p += 2;
    }
    memcpy(p, suffix, suffixLen);
    p += suffixLen;
    *p = '\0';

    // Send the uplink command and data
    err = modemSend(NULL, cmd);
    if (err) {
        memFree(cmd);
        return err;
    }

    // Done
    memFree(cmd);
    return errNone;
}

// Handle a downlink
err_t workModemDownlink(J *hexData, uint8_t *unused, uint32_t unusedLen)
{

    char *hex = JGetStringValue(hexData);
    uint32_t payloadLen = strlen(hex)/2;
    uint8_t *payload;
    err_t err = memAlloc(payloadLen, &payload);
    if (err) {
        return err;
    }

    stoh(hex, payload, payloadLen);

    serialSendMessageToNotecard(serialCreateMessage(ReqDownlink, NULL, NULL, payload, payloadLen));
    return errNone;

}
