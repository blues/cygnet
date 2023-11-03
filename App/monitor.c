
#include "app.h"

// Have we yet received a 'hello' from the service?
#define SEND_HELLO_SECS         5
bool receivedHello = false;
int64_t nextHelloDueMs = 0;

// Monitor poller - return 0 if wait forever, else ms of the latest to come back
uint32_t monitor(void)
{
    uint32_t returnInMs = ms1Day;

    // If we haven't yet received a hello, poll on a periodic basis and send the
    // notecard a 'hello' message introducing ourselves.
    if (!receivedHello) {
        if (timerMs() >= nextHelloDueMs) {
            nextHelloDueMs = timerMs() + (SEND_HELLO_SECS * ms1Sec);
            if (modemId[0] != '\0' && modemVersion[0] != '\0') {
                J *body = JParse(osBuildConfig());
                if (body == NULL) {
                    body = JCreateObject();
                }
                JAddStringToObject(body, "id", modemId);
                JAddStringToObject(body, "modem", modemVersion);
                serialSendMessageToNotecard(serialCreateMessage(ReqHello, body, NULL, 0));
            }
        }
        returnInMs = GMIN(returnInMs, nextHelloDueMs - timerMs());
    }


    // Done
    return returnInMs;

}

// Received the hello
void monitorReceivedHello(void)
{
    receivedHello = true;
}
