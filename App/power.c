
#include <stdarg.h>
#include "app.h"
#include "serial.h"
#include "usart.h"

// Power state
bool modemPoweredOn = false;
bool gpsPoweredOn = false;
bool mainPoweredOn = false;
uint32_t powerNeeds = 0;
bool modemIsReady = false;

// Turn on modem power
err_t powerOn(uint32_t reason)
{
    err_t err;
    arrayString results;

    // Add a reason
    powerNeeds |= reason;

    // Set the power pins appropriately
    bool turnGpsOn = ((powerNeeds & POWER_GPS) != 0);
    bool turnModemOn = ((powerNeeds & (POWER_DATA|POWER_INIT|POWER_MODEM_DFU)) != 0);
    bool turnMainOn = turnGpsOn || turnModemOn;
    if (turnMainOn && !mainPoweredOn) {
        if (!turnModemOn && !modemPoweredOn) {
            HAL_GPIO_WritePin(MODEM_POWER_NOD_GPIO_Port, MODEM_POWER_NOD_Pin, GPIO_PIN_SET);
        }
        HAL_GPIO_WritePin(MAIN_POWER_GPIO_Port, MAIN_POWER_Pin, GPIO_PIN_SET);
        mainPoweredOn = true;
    }
    if (turnGpsOn && !gpsPoweredOn) {
        gpsPoweredOn = true;
        debugf("gps: powered on\n");
    }

    // If the modem doesn't need to be powered on, our job is done
    if (!turnModemOn) {
        return errNone;
    }

    // If the modem needs to be powered-on but it's already on, exit
    if (modemPoweredOn) {
        return errNone;
    }

    // Clear all pending URCs before we power-on
    modemUrcRemove(NULL);

    // Init the USART before we power-on, so we can capture all output
#ifndef MODEM_ALWAYS_ON
    MX_USART2_UART_Init(false, 115200);
#endif

    // Physically turn on the power
#ifndef MODEM_ALWAYS_ON

    // Make sure we select the SIM as appropriate
    bool externalSimInserted = (HAL_GPIO_ReadPin(SIM_NPRESENT_GPIO_Port, SIM_NPRESENT_Pin) == GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SEL_SIM_GPIO_Port, SEL_SIM_Pin, externalSimInserted ? GPIO_PIN_RESET : GPIO_PIN_SET);

    // Make sure RESET# and PWRKEY# are both high before we power-on
    HAL_GPIO_WritePin(MODEM_RESET_NOD_GPIO_Port, MODEM_RESET_NOD_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(MODEM_PWRKEY_NOD_GPIO_Port, MODEM_PWRKEY_NOD_Pin, GPIO_PIN_SET);

    // Power-on the modem
    timerMsSleep(100);
    HAL_GPIO_WritePin(MODEM_POWER_NOD_GPIO_Port, MODEM_POWER_NOD_Pin, GPIO_PIN_RESET);

    // Wait for the modem to stabilize and then drive PWRKEY low for at least 500ms
    timerMsSleep(750);
    HAL_GPIO_WritePin(MODEM_PWRKEY_NOD_GPIO_Port, MODEM_PWRKEY_NOD_Pin, GPIO_PIN_RESET);
    timerMsSleep(1500);
    HAL_GPIO_WritePin(MODEM_PWRKEY_NOD_GPIO_Port, MODEM_PWRKEY_NOD_Pin, GPIO_PIN_SET);

#endif // !MODEM_ALWAYS_ON

    // If the modem is always on, do a reset to simulate power-on
#ifdef MODEM_ALWAYS_ON
    timerMsSleep(1000);
    serialSendLineToModem("AT+QRST=1");
    timerMsSleep(750);
#endif

    // Physically power-on the modem
    modemIsReady = false;
    modemPoweredOn = true;
    debugf("modem: powered on\n");

    // If we're doing a modem DFU, we're done.
    if ((powerNeeds & POWER_MODEM_DFU) != 0) {
        return errNone;
    }

    // Send the most critical startup command to the modem, which is
    // to disable the 10-second timer.  These are sent even before
    // waiting for RDY just as a defensive mechanism for slowdowns.
    serialSendLineToModem("AT+QSCLK=0");

    // Wait until RDY, flushing everything else that comes in
    bool ready = false;
    int64_t timeoutMs = timerMs() + (30 * ms1Sec);
    while (!ready) {
        if (timerMs() >= timeoutMs) {
            powerOff(reason);
            return errF("modem power-on timeout");
        }
        ready = modemHasReceivedLine("RDY");
    }

    // Let things settle a bit after power-on
    timerMsSleep(2500);

    // Turn off echo
    err = errNone;
    for (int i=0; i<6; i++) {
        err = modemSend(NULL, "ATE0");
        if (!err) {
            break;
        }
        timerMsSleep(i < 3 ? 750 : 2500);
    }
    if (err) {
        powerOff(reason);
        return err;
    }

    // Indicate that the modem is ready to take AT commands via modemSend
    modemIsReady = true;

    // Turn off 10 second auto sleep timer
    err = modemSend(NULL, "AT+QSCLK=0");
    if (err) {
        powerOff(reason);
        return err;
    }

    // Enable LED
    modemSend(NULL, "AT+QLEDMODE=1");
        modemSend(NULL, "AT");//OZZIE

    // Get IMSI if necessary
    if (configModemId[0] == '\0') {
        err = modemSend(&results, "AT+CIMI");
        err = modemRequireResults(&results, err, 1, "can't get modem IMSI");
        if (err) {
            powerOff(reason);
            return err;
        }
        strLcpy(configModemId, STARNOTE_ID_SCHEME);
        strLcat(configModemId, (char *) arrayEntry(&results, 0));
        arrayReset(&results);
    }

    // Get firmware version
    if (configModemVersion[0] == '\0') {
        err = modemSend(&results, "AT+QGMR");
        err = modemRequireResults(&results, err, 1, "can't get modem firmware version");
        if (err) {
            powerOff(reason);
            return err;
        }
        strLcpy(configModemVersion, (char *) arrayEntry(&results, 0));
        arrayReset(&results);
    }

    // Done
    return errNone;

}

// Turn off power
void powerOff(uint32_t reason)
{

    // Remove reasons.  If we still need to stay on, exit
    powerNeeds &= ~reason;

    // Set the power pins appropriately
    bool turnGpsOff = ((powerNeeds & POWER_GPS) == 0);
    bool turnModemOff = ((powerNeeds & (POWER_DATA|POWER_INIT)) == 0);

    // Exit if already powered off
    if (turnModemOff && modemPoweredOn) {

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

        // Do a soft power-down of the modem, giving it a bit to flush
#ifndef MODEM_ALWAYS_ON
        modemSend(NULL, "AT+QPOWD=0");
        timerMsSleep(3000);
#endif

        // Uninit the USART
#ifndef MODEM_ALWAYS_ON
        MX_USART2_UART_DeInit();
#endif

        // Physically turn off the power
#ifndef MODEM_ALWAYS_ON

        // Make sure we leave RESET# and PWRKEY# high
        HAL_GPIO_WritePin(MODEM_RESET_NOD_GPIO_Port, MODEM_RESET_NOD_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(MODEM_PWRKEY_NOD_GPIO_Port, MODEM_PWRKEY_NOD_Pin, GPIO_PIN_SET);

        // Power-off the modem
        HAL_GPIO_WritePin(MODEM_POWER_NOD_GPIO_Port, MODEM_POWER_NOD_Pin, GPIO_PIN_SET);

        // Make sure we leave SEL_SIM low, to discharge lingering power in the modem through the pull-up
        HAL_GPIO_WritePin(SEL_SIM_GPIO_Port, SEL_SIM_Pin, GPIO_PIN_RESET);

#endif // MODEM_ALWAYS_ON

        // We're powered off.
        modemPoweredOn = false;
        modemIsReady = false;
        modemSetConnected(false);
        debugf("modem: powered off\n");

    }

    // If GPS needs to be powered down, power it down
    if (turnGpsOff && gpsPoweredOn) {
        gpsPoweredOn = false;
        debugf("gps: powered off\n");
    }

    // If neither the modem or GPS needs power, turn off MAIN
    if (!gpsPoweredOn && !modemPoweredOn && mainPoweredOn) {
        HAL_GPIO_WritePin(MAIN_POWER_GPIO_Port, MAIN_POWER_Pin, GPIO_PIN_RESET);
        mainPoweredOn = false;
    }

}
