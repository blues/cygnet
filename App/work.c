
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

// Request that the modem be powered on
err_t workModemRequest(J *body, uint8_t *payload, uint32_t payloadLen)
{

    // Power-on the modem, and then power it off, to get the parameters
    err_t err = modemPowerOn();
    if (err) {
        return err;
    }

    // Set the current lat/lon in the modem
    double lat, lon;
    if (!locGet(&lat, &lon, NULL)) {
        return errF("location not set");
    }
    err = modemSend(NULL, "AT+QGNSSINFO=%f,%f,0,0,0", lat, lon);
    if (err) {
        return err;
    }

    // Done
    return errNone;

}

// Request that the modem be powered off
err_t workModemRelease(J *body, uint8_t *payload, uint32_t payloadLen)
{
    modemPowerOff();
    return errNone;
}

