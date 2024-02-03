
#include "app.h"

// Have we yet received a 'hello' from the service?
#define SEND_HELLO_SECS         10
STATIC int64_t nextHelloDueMs = 0;

// Monitor poller - return 0 if wait forever, else ms of the latest to come back
uint32_t monitor(void)
{
    uint32_t returnInMs = 0xffff;

    // If we don't have our modem info yet, try to fetch it
    if (modemInfoNeeded()) {
        err_t err = powerOn(POWER_INIT);
        if (!err) {
            powerOff(POWER_INIT);
        }
    }

    // If we haven't yet received a hello, poll on a periodic basis and send the
    // notecard a 'hello' message introducing ourselves.
    if (!configReceivedHello) {
        if (timerMs() >= nextHelloDueMs) {
            nextHelloDueMs = timerMs() + (SEND_HELLO_SECS * ms1Sec);
            returnInMs = GMIN(returnInMs, (uint32_t) (nextHelloDueMs - timerMs()));
            if (configModemId[0] != '\0' && configModemVersion[0] != '\0') {
                J *body = JDuplicate(postGetCachedTestCert(), true);
                serialSendMessageToNotecard(serialCreateMessage(ReqHello, NULL, body, NULL, 0));
            }
        }
    }

    // Done
    return returnInMs;

}
