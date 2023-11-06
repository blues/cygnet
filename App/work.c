
#include "app.h"

// Connect that the modem be powered on
err_t workModemConnect(J *body, uint8_t *payload, uint32_t payloadLen)
{
    err_t err;

    // If we haven't yet received a hello, we must wait
    if (!monitorHaveReceivedHello()) {
        err = modemPowerOn();
        if (err) {
            return err;
        }
        modemPowerOff();
        timerMsSleep(5000);
        if (!monitorHaveReceivedHello()) {
            return errF("haven't yet successfully heard from notecard");
        }
    }

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
    err = modemPowerOn();
    if (err) {
        return err;
    }

    // Disable radio.  This is required before executing a QFLOCK request,
    // as well as things related to band, APN, etc.
    err = modemSend(NULL, "AT+CFUN=0");
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

    // Remove lock.  I don't know why, but Skylo recommends
    // removing the lock before re-activating it
    err = modemSend(NULL, "AT+QLOCKF=0");
    if (err) {
        modemPowerOff();
        return err;
    }

    // Lock to the specific NB-IoT frequency and channel
    int skyloEarfcn = 7697;         // For The Rocks
    int skyloEarfcnOffset = 2;      // For The Rocks
    err = modemSend(NULL, "AT+QLOCKF=1,%d,%d", skyloEarfcn, skyloEarfcnOffset);
    if (err) {
        modemPowerOff();
        return err;
    }

    // Set to NIDD and set our APN
    char *skyloApn = "blues.demo";    // For Blues
    err = modemSend(NULL, "AT+QCGDEFCONT=\"Non-IP\",\"%s\"", skyloApn);
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

    // Make sure that the module knows our location
    err = modemSend(NULL, "AT+QGNSSINFO=%f,%f,0,0,0", lat, lon);
    if (err) {
        modemPowerOff();
        return err;
    }

    // Wait for the registration to complete
    bool registered = false;
    int secs = 0;
    int maxSecs = 90;
    int64_t regExpiresMs = timerMs() + ((int64_t)maxSecs * ms1Sec);
    while (timerMs() < regExpiresMs) {
        if (modemUrc("+CEREG: 1", false) || modemUrc("+CEREG: 5", false) || modemUrc("+CEREG: 1,1", false) || modemUrc("+CEREG: 1,5", false)) {
            registered = true;
            break;
        }
        timerMsSleep(1000);
        if ((++secs % 5) == 0) {
            debugf("waiting for network registration (%d/%d)\n", secs, maxSecs);
        }
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

    // Prefix and suffix for hex string
    char prefix[40];
    snprintf(prefix, sizeof(prefix), "AT+CSODCP=1,%d,\"", payloadLen);
    uint32_t prefixLen = strlen(prefix);
    char *suffix = "\"";
    uint32_t suffixLen = strlen(suffix);

    // Allocate the hex buffer
    char *cmd;
    err_t err = memAlloc(prefixLen + (payloadLen*2) + suffixLen, &cmd);
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
