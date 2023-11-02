
#include "app.h"

// One-time initialization
err_t workInit(J *unused)
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
err_t workModemRequest(J *req)
{

    // Power-on the modem, and then power it off, to get the parameters
    err_t err = modemPowerOn();
    if (err) {
        return err;
    }

    // Set the current lat/lon in the modem
    err = modemSend(NULL, "AT+QGNSSINFO=%f,%f,0,0,0", JGetNumber(req, "lat"), JGetNumber(req, "lon"));
    if (err) {
        return err;
    }

    // Done
    return errNone;

}

// Request that the modem be powered off
err_t workModemRelease(J *req)
{
    modemPowerOff();
    return errNone;
}

