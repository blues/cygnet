
#include "notelib.h"
#include "serial.h"
#include "request.h"

#ifdef DEBUG_TIMERS
#include "timer_if.h"
#endif

// CRC
#define CRC_FIELD_LENGTH		22	// ,"crc":"SSSS:CCCCCCCC"
#define	CRC_FIELD_NAME_OFFSET	1
#define	CRC_FIELD_NAME_TEST		"\"crc\":\""
bool crcError(uint8_t *json, uint32_t *ioJsonLen, bool *hasCrc, uint16_t *reqCrcSeqno);
void crcAdd(uint8_t **ioJson, uint32_t *ioJsonLen, uint16_t seqno);

// A button was pressed
STATIC bool processButtonPress = false;

// Perform work after sending reply
uint32_t reqDeferredWork = rdtNone;

// Forwards
bool processReq(UART_HandleTypeDef *huart);
bool processButton(void);

// Request task
void reqTask(void *params)
{

    // Init task
    taskRegister(TASKID_REQ, TASKNAME_REQ, TASKLETTER_REQ, TASKSTACK_REQ);

    // Tell the serial subsystem what our taskID is, so it can awaken us
    serialSetReqTaskID(TASKID_REQ);

    // Loop, extracting requests from the serial port and processing them
    while (true) {
        bool didSomething = false;
        didSomething |= processReq(&huart1);
        didSomething |= processReq(&hlpuart1);
        didSomething |= processReq(&huart_i2cs);
        didSomething |= processButton();
        if (!didSomething) {
#ifdef DEBUG_TIMERS
            static int64_t msSinceScheduled = 0;
            static int64_t msBootFirstIter = 0;
            if (msBootFirstIter == 0) {
                msBootFirstIter = timerMsSinceBoot();
            }
            taskTake(TASKID_REQ, 10*ms1Sec);
            msSinceScheduled += 10*ms1Sec+450;  // ~300ms of inaccuracy
            debugf("timer:%lld scheduler:%lld\n", timerMsSinceBoot()-msBootFirstIter, msSinceScheduled);
#else
            taskTake(TASKID_REQ, ms1Day);
#endif
        }
    }

}

// Process incoming serial request
bool processReq(UART_HandleTypeDef *huart)
{

    // Get the pending JSON request
    uint8_t *reqJSON;
    uint32_t reqJSONLen;
    bool diagAllowed;
    if (!serialLock(huart, &reqJSON, &reqJSONLen, &diagAllowed)) {
        return false;
    }

    // If it's a 0-length request, we must output our standard \r\n response
    // because this is critical for note-c to answer "are you there?"
    if (reqJSONLen == 0) {
        serialUnlock(huart, true);
        serialOutputLn(huart, NULL, 0);
        if (reqJSON != NULL) {
            memFree(reqJSON);
        }
        return true;
    }

    // See if this JSON contains a CRC, and remove it if it does.
    err_t err = errNone;
    bool requestHasCrc;
    uint16_t requestCrcSeqno;
    if (crcError(reqJSON, &reqJSONLen, &requestHasCrc, &requestCrcSeqno)) {
        err = errF("CRC error in request " ERR_CRC " " ERR_IO);
    }

    // Busy LED
    ledBusy(true);

    // Process the request
    uint8_t *rspJSON = NULL;
    uint32_t rspJSONLen;
    if (!err) {
        err = reqProcess(huart == &huart1, reqJSON, reqJSONLen, diagAllowed, &rspJSON, &rspJSONLen);
    }
    serialUnlock(huart, true);
    if (!err && requestHasCrc) {
        crcAdd(&rspJSON, &rspJSONLen, requestCrcSeqno);
    }
    if (err) {
        if (rspJSON != NULL) {
            memFree(rspJSON);
        }
        char *errstr = errString(err);
        if (errstr[0] == '+') {
            serialOutputLn(huart, (uint8_t *) errstr, strlen(errstr));
        } else {
            err = errBody(err, &rspJSON, &rspJSONLen);
            if (!err) {
                if (requestHasCrc) {
                    crcAdd(&rspJSON, &rspJSONLen, requestCrcSeqno);
                }
                serialOutputLn(huart, rspJSON, rspJSONLen);
                if (config.traceRequests || configp.traceRequests) {
                    debugMessage("<< ");
                    debugMessageLen((char *)rspJSON, rspJSONLen);
                    debugMessage("\n");
                }
                memFree(rspJSON);
            }
        }
    } else {
        if (rspJSON != NULL) {
            serialOutputLn(huart, rspJSON, rspJSONLen);
            memFree(rspJSON);
        } else {
            // On I2C3, we need to notify the driver that we have completed receiving
            // so it can be in an idle state.  This is important in case what we've
            // received is a 'cmd' rather than a 'req', because in that case we won't be
            // transmitting anything back to the notecard and so if we didn't do this
            // the MY_I2C3_Watchdog in the serial poller would eventually reset the
            // i2c3 port.  Worse, this keeps the processor from entering STOP2 mode
            // for about 15-20 seconds when a Sleep command is sent to us.
            MY_I2CS_ReceiveCompleted();
        }
    }

    // Busy LED
    ledBusy(false);

    // Perform deferred work
    if (reqDeferredWork != rdtNone) {
        timerMsSleep(1500);
        switch (reqDeferredWork) {
        case rdtRestart:
            debugPanic(ReqCardRestart);
            break;
        case rdtBootloader:
            MY_JumpToBootloaderDirect();
            debugPanic("boot");
            break;
        case rdtRestore:
            pageFactoryReset();
            debugPanic(ReqCardRestore);
            break;
        }
    }

    // Processed
    return true;

}

// See if there's a CRC error in the supplied JSON, and strip it regardless
bool crcError(uint8_t *ijson, uint32_t *ioJsonLen, bool *hasCrc, uint16_t *reqCrcSeqno)
{
    *hasCrc = false;
    *reqCrcSeqno = 0;
    char *json = (char *) ijson;
    uint32_t jsonLen = *ioJsonLen;
    if (jsonLen < CRC_FIELD_LENGTH+2 || json[jsonLen-1] != '}') {
        return false;
    }
    size_t fieldOffset = ((jsonLen-1) - CRC_FIELD_LENGTH);
    if (memcmp(&json[fieldOffset+CRC_FIELD_NAME_OFFSET], CRC_FIELD_NAME_TEST, sizeof(CRC_FIELD_NAME_TEST)-1) != 0) {
        return false;
    }
    *hasCrc = true;
    char *p = &json[fieldOffset + CRC_FIELD_NAME_OFFSET + (sizeof(CRC_FIELD_NAME_TEST)-1)];
    *reqCrcSeqno = (uint16_t) atoh(p, 4);
    uint32_t actualCrc32 = (uint32_t) atoh(p+5, 8);
    json[fieldOffset++] = '}';
    json[fieldOffset] = '\0';
    *ioJsonLen = fieldOffset;
    uint32_t shouldBeCrc32 = crc32(json, fieldOffset);
    return (shouldBeCrc32 != actualCrc32);
}

// Add a CRC to the input JSON if possible.  If not, no big deal.
void crcAdd(uint8_t **ioJson, uint32_t *ioJsonLen, uint16_t seqno)
{
    // Allocate a block the size of the input json plus the size of
    // the field to be added.  Note that the input JSON ends in '"}' and
    // this will be replaced with a combination of 4 hex digits of the
    // seqno plus 4 hex digits of the CRC32, and the '}' will be
    // transformed into ',"crc":"SSSSCCCCCCCC"}' where SSSS is the
    // seqno and CCCCCCCC is the crc32.  Note that the comma is
    // replaced with a space if the input json doesn't contain
    // any fields, so that we always return compliant JSON.
    char *json = (char *) (*ioJson);
    size_t jsonLen = (size_t) (*ioJsonLen);
    if (jsonLen < 2 || json[jsonLen-1] != '}') {
        return;
    }
    char *newJson;
    err_t err = memAlloc(jsonLen+CRC_FIELD_LENGTH, &newJson);
    if (err) {
        return;
    }
    bool isEmptyObject = (memchr(json, ':', jsonLen) == NULL);
    size_t newJsonLen = jsonLen-1;
    memcpy(newJson, json, newJsonLen);
    newJson[newJsonLen++] = (isEmptyObject ? ' ' : ',');	// Replace }
    newJson[newJsonLen++] = '"';							// +1
    newJson[newJsonLen++] = 'c';							// +2
    newJson[newJsonLen++] = 'r';							// +3
    newJson[newJsonLen++] = 'c';							// +4
    newJson[newJsonLen++] = '"';							// +5
    newJson[newJsonLen++] = ':';							// +6
    newJson[newJsonLen++] = '"';							// +7
    htoa16(seqno, (uint8_t *) &newJson[newJsonLen]);
    newJsonLen += 4;										// +11
    newJson[newJsonLen++] = ':';							// +12
    htoa32(crc32(json, jsonLen), &newJson[newJsonLen]);
    newJsonLen += 8;										// +20
    newJson[newJsonLen++] = '"';							// +21
    newJson[newJsonLen++] = '}';							// +22 == CRC_FIELD_LENGTH
    *ioJson = (uint8_t *) newJson;
    *ioJsonLen = (uint32_t) newJsonLen;
    memFree(json);
}

// Note that the button was pressed
void reqButtonPressedISR()
{
    processButtonPress = true;
    taskGiveFromISR(TASKID_REQ);
}

// Process button press
bool processButton()
{
    if (!processButtonPress) {
        return false;
    }
    processButtonPress = false;

    // Give positive indication
    int iterations = 2;
    for (int i=0; i<iterations; i++) {
        HAL_GPIO_WritePin(LED_BUSY_GPIO_Port, LED_BUSY_Pin, GPIO_PIN_SET);
        ledEnable(FLED_BUTTON_CONFIRM, true);
        timerMsSleep(100);
        HAL_GPIO_WritePin(LED_BUSY_GPIO_Port, LED_BUSY_Pin, GPIO_PIN_RESET);
        ledEnable(FLED_BUTTON_CONFIRM, false);
        if (i != iterations-1) {
            timerMsSleep(100);
        }
    }

    // Enqueue a ping
    syncPing();

    // Done
    return true;

}
