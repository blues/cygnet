// Copyright 2024 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "app.h"
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

    // Loop, extracting requests from the serial ports and processing them
    while (true) {

        bool didSomething = false;
        didSomething |= processReq(&hlpuart1);
        didSomething |= processReq(&huart1);
        didSomething |= processReq(NULL);
        didSomething |= processButton();
        if (!didSomething) {
            taskTake(TASKID_REQ, ms1Hour);
        }
    }

}

// Process incoming serial request
bool processReq(UART_HandleTypeDef *huart)
{
    err_t err;

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
    ledEnable(true);

    // Process the request (which is conveniently null-terminated by the serial subsystem)
    if (serialIsDebugPort(huart)) {
        serialSetDebugPort(huart);
        bool debugWasEnabled = MX_DBG_Enable(false);
        err = reqProcess(true, reqJSON, diagAllowed);
        MX_DBG_Enable(debugWasEnabled);
    } else {
        err = reqProcess(false, reqJSON, diagAllowed);
    }
    serialUnlock(huart, true);
    if (err) {
        char *errstr = errString(err);
        serialOutputLn(huart, (uint8_t *) errstr, strlen(errstr));
    }

    // Busy LED
    ledEnable(false);

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
        ledEnable(true);
        timerMsSleep(100);
        ledEnable(false);
        if (i != iterations-1) {
            timerMsSleep(100);
        }
    }

    // Done
    return true;

}
