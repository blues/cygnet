
#include "app.h"
#include "serial.h"
#include "usart.h"

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
        didSomething |= processButton();
        if (!didSomething) {
            taskTake(TASKID_REQ, ms1Day);
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

    // Busy LED
    ledBusy(true);

    // Process the request
    uint8_t *rspJSON = NULL;
    uint32_t rspJSONLen;
    err_t err = reqProcess(huart == &huart1, reqJSON, reqJSONLen, diagAllowed, &rspJSON, &rspJSONLen);
    serialUnlock(huart, true);
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
                serialOutputLn(huart, rspJSON, rspJSONLen);
                debugMessage("<< ");
                debugMessageLen((char *)rspJSON, rspJSONLen);
                debugMessage("\n");
                memFree(rspJSON);
            }
        }
    } else {
        if (rspJSON != NULL) {
            serialOutputLn(huart, rspJSON, rspJSONLen);
            memFree(rspJSON);
        }
    }

    // Busy LED
    ledBusy(false);

    // Perform deferred work
    if (reqDeferredWork != rdtNone) {
        timerMsSleep(1500);
        switch (reqDeferredWork) {
        case rdtRestart:
            debugPanic("restart");
            break;
        case rdtBootloader:
            MX_JumpToBootloader();
            debugPanic("boot");
            break;
        }
    }

    // Processed
    return true;

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

    // Give positive indication that we're still alive
    int iterations = 2;
    for (int i=0; i<iterations; i++) {
        HAL_GPIO_WritePin(LED_BUSY_GPIO_Port, LED_BUSY_Pin, GPIO_PIN_SET);
        timerMsSleep(100);
        HAL_GPIO_WritePin(LED_BUSY_GPIO_Port, LED_BUSY_Pin, GPIO_PIN_RESET);
        if (i != iterations-1) {
            timerMsSleep(100);
        }
    }

    // Done
    return true;

}