

#include "notelib.h"
#include "serial.h"
#include "main.h"
#include "task.h"
#include "queue.h"
#include "utilities.h"
#include "stm32_systime.h"
#include "stm32_lpm.h"
#include "timer_if.h"
#include "LmHandler.h"
#include "utilities_def.h"
#include "lorawan_conf.h"
#include "lora_conf.h"

// Forwards
void loraTask(void *params);

// Main task
void mainTask(void *params)
{

    // Init task
    taskRegister(TASKID_MAIN, TASKNAME_MAIN, TASKLETTER_MAIN, TASKSTACK_MAIN);

    // Init timer manager
    UTIL_TIMER_Init();

    // Init low power manager
    UTIL_LPM_Init();
    UTIL_LPM_SetOffMode((1 << CFG_LPM_APPLI_Id), UTIL_LPM_DISABLE);
    if (osDebugging()) {
        UTIL_LPM_SetStopMode((1 << CFG_LPM_APPLI_Id), UTIL_LPM_DISABLE);
    }

    // Remember the timer value when we booted
    timerSetBootTime();

    // Initialize serial, thus enabling debug output
    serialInit(TASKID_MAIN);

    // Enable debug info at startup, by policy
    MX_DBG_Enable(true);

    // Display usage stats
    debugf("\n");
    debugf("**********************\n");
    char timestr[48];
    uint32_t s = timeSecs();
    timeDateStr(s, timestr, sizeof(timestr));
    if (timeIsValid()) {
        debugf("%s UTC (%d)\n", timestr, s);
    }
    uint32_t pctUsed = ((heapPhysical-heapFreeAtStartup)*100)/65536;
    debugf("RAM: %lu free of %lu (%d%% used)\n", (unsigned long) heapFreeAtStartup, heapPhysical, pctUsed);
    extern void *ROM_CONTENT$$Limit;
    uint32_t imageSize = (uint32_t) (&ROM_CONTENT$$Limit) - FLASH_BASE;
    pctUsed = (imageSize*100)/262144;
    debugf("ROM: %lu free of 256KB (%d%% used)\n", (unsigned long) (262144 - imageSize), pctUsed);
    debugf("**********************\n");

    // Signal that we've started, but do it here so we don't block request processing
    ledRestartSignal();

    // Load config early because it sets up LoRaWAN defaults, sets up AUX for debugging, etc.
    configpLoad();
    configLoad();

    // Init LEDs now that config is loaded
    ledInit();

    // Create the lora processing task
#ifndef DEBUG_DISABLE_SYNC
    xTaskCreate(loraTask, TASKNAME_LORA, STACKWORDS(TASKSTACK_LORA), NULL, TASKPRI_LORA, NULL);
#endif

    // Create the serial request processing task
    xTaskCreate(reqTask, TASKNAME_REQ, STACKWORDS(TASKSTACK_REQ), NULL, TASKPRI_REQ, NULL);

    // Poll, moving serial data from interrupt buffers to app buffers
    for (;;) {
        serialPoll();
    }

}

// Lora task
void loraTask(void *params)
{

    // Init task
    taskRegister(TASKID_LORA, TASKNAME_LORA, TASKLETTER_LORA, TASKSTACK_LORA);

    // Init the LoRaWAN app processing
    loraAppInit();

    // Create the queued transmit task
    xTaskCreate(syncTask, TASKNAME_SYNC, STACKWORDS(TASKSTACK_SYNC), NULL, TASKPRI_SYNC, NULL);

    // Process events
    for (;;) {
        taskTake(TASKID_LORA, TASK_WAIT_FOREVER);
        loraAppProcess();
    }

}