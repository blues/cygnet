
#include "app.h"

// Have we yet received a 'hello' from the service?
#define SEND_HELLO_SECS         5
bool receivedHello = false;
int64_t nextHelloDueMs = 0;

// Monitor poller - return 0 if wait forever, else ms of the latest to come back
uint32_t monitor(void)
{
    uint32_t returnInMs = ms1Day;

    // If we haven't yet received a hello, send one 
    if (!receivedHello) {
        if (timerMs() >= nextHelloDueMs) {

            J *body = JCreateObject();
            JAddStringToObject(body, FieldReq, ReqHello);
            JAddStringToObject(body, "id", STARNOTE_SCHEME ":" "<IMSI-GOES-HERE>");
            JAddStringToObject(body, "modem", "<MODEM-FIRMWARE-GOES-HERE>");
            J *firmware = JParse(osBuildConfig());
            if (firmware != NULL) {
                JAddItemToObject(body, "firmware", firmware);
            }

            J *msg = JCreateObject();
            JAddStringToObject(msg, "cmd", "card.ntn");
            JAddItemToObject(msg, "body", body);

            serialOutputObjectToNotecard(msg);
            nextHelloDueMs = timerMs() + (SEND_HELLO_SECS * ms1Sec);
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
