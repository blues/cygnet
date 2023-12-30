
#include "app.h"

// Have we yet received a 'hello' from the service?
#define SEND_HELLO_SECS         5
STATIC int64_t nextHelloDueMs = 0;

// Monitor poller - return 0 if wait forever, else ms of the latest to come back
uint32_t monitor(void)
{
    uint32_t returnInMs = 0xffff;

    // If we don't have our modem info yet, try to fetch it
    ozzie();
    ozzie();
    ozzie();
    if (modemInfoNeeded()) {
        ozzie();
        err_t err = powerOn(POWER_INIT);
        ozzie();
        if (!err) {
            powerOff(POWER_INIT);
        }
        ozzie();
    }

    // If we haven't yet received a hello, poll on a periodic basis and send the
    // notecard a 'hello' message introducing ourselves.
        ozzie();
    if (!configReceivedHello) {
        if (timerMs() >= nextHelloDueMs) {
            nextHelloDueMs = timerMs() + (SEND_HELLO_SECS * ms1Sec);
            returnInMs = GMIN(returnInMs, (uint32_t) (nextHelloDueMs - timerMs()));
            if (configModemId[0] != '\0' && configModemVersion[0] != '\0') {
                J *body = JParse(osBuildConfig());
                if (body == NULL) {
                    body = JCreateObject();
                }
                JAddStringToObject(body, STARNOTE_ID_FIELD, configModemId);
                JAddIntToObject(body, STARNOTE_CID_FIELD, STARNOTE_CID_TYPE);
                JAddStringToObject(body, STARNOTE_MODEM_FIELD, configModemVersion);
                JAddStringToObject(body, STARNOTE_POLICY_FIELD, configPolicy);
                JAddIntToObject(body, STARNOTE_MTU_FIELD, configMtu);
                JAddStringToObject(body, STARNOTE_SKU_FIELD, configSku);
                JAddStringToObject(body, STARNOTE_OC_FIELD, configOc);
                JAddStringToObject(body, STARNOTE_APN_FIELD, configApn);
                JAddStringToObject(body, STARNOTE_BAND_FIELD, configBand);
                JAddStringToObject(body, STARNOTE_CHANNEL_FIELD, configChannel);
                serialSendMessageToNotecard(serialCreateMessage(ReqHello, NULL, body, NULL, 0));
            }
        }
    }


    // Done
    return returnInMs;

}
