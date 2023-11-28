
#include "app.h"
#include "serial.h"
#include "usart.h"

// A button was pressed
STATIC bool processButtonPress = false;

// Perform work after sending reply
uint32_t reqDeferredWork = rdtNone;
bool reqActive = true;

// Forwards
bool processReq(UART_HandleTypeDef *huart);
bool processButton(void);

// Request task
void reqTask(void *params)
{
    int64_t lastDidSomethingMs = timerMs();

    // Init task
    taskRegister(TASKID_REQ, TASKNAME_REQ, TASKLETTER_REQ, TASKSTACK_REQ);

    // Loop, extracting requests from the serial port and processing them
    while (true) {

        // Note that we are busy servicing a request
        reqActive = true;

        // Process requests
        bool didSomething = false;
        didSomething |= processReq(&hlpuart1);
        didSomething |= processReq(&huart1);
        didSomething |= processButton();
        if (didSomething) {
            lastDidSomethingMs = timerMs();
        } else {
            int64_t sleepMs;

            // Keep us from going into and out of STOP2 mode on a crazy high frequency
            // basis, just to prevent thrashing.
            if ((uint32_t) (timerMs() - lastDidSomethingMs) > 3000) {
                sleepMs = ms1Day;
                reqActive = false;
            } else {
                sleepMs = 100;
            }

            // Wait for something to appear on one of the request ports
            taskTake(TASKID_REQ, sleepMs);

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
        return true;
    }

    // Busy LED
    ledBusy(true);

    // Process the request
    J *rsp;
    err_t err = reqProcess(huart == &huart1, reqJSON, reqJSONLen, diagAllowed, &rsp);
    serialUnlock(huart, true);
    if (err) {
        char *reqstr = "\"req\":\"";
        if (memmem(reqJSON, reqJSONLen, reqstr, strlen(reqstr)) != NULL) {
            uint8_t *rspJSON = NULL;
            uint32_t rspJSONLen = 0;
            errBody(err, &rspJSON, &rspJSONLen);
            serialOutputLn(huart, rspJSON, rspJSONLen);
            memFree(rspJSON);
        }
    } else {
        serialOutputObject(huart, rsp);
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
