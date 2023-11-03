
#include "app.h"

// One-time initialization
err_t workInit(J *body, uint8_t *payload, uint32_t payloadLen)
{
    err_t err;

    // Power-on the modem, and then power it off, to get the parameters
    err = modemPowerOn();
    if (err) {
        return err;
    }
    modemPowerOff();

    // Done
    return errNone;

}

// Connect that the modem be powered on
err_t workModemConnect(J *body, uint8_t *payload, uint32_t payloadLen)
{
    arrayString results;

    // The location will be needed below
    double lat, lon;
    if (!locGet(&lat, &lon, NULL)) {
        return errF("location not available");
    }

    // Power-on the modem, and then power it off, to get the parameters
    err_t err = modemPowerOn();
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

    // This command is used by the MCU to actively set and query the current GNSS position information of the
    // terminal. The module will use the GNSS position information inputted by this command for satellite
    // communication timing and frequency compensation. When the module needs GNSS position information,
    // it will actively report URC +QGNSSINFO: 1 to notify the MCU when to input GNSS position information
    // to the module, or +GNSSINFO: 0 to notify that the MCU no longer needs to provide GNSS information.
    err = modemSend(NULL, "AT+QGNSSINFO=%f,%f,0,0,0", lat, lon);
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
        err = modemSend(&results, "AT+CPIN?");
        if (!err) {
            if (modemResult(&results, "+CPIN: READY") == -1) {
                err = errF("SIM not ready");
            }
        }
        arrayReset(&results);
        if (!err) {
            break;
        }
        if (i >= 10) {
            modemPowerOff();
            return errF("SIM not ready");
        }
        timerMsSleep(750);
    }

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
