// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

// Definitions for defaultTask
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
    .name = "defaultTask",
    .stack_size = 128 * 4,
    .priority = (osPriority_t) osPriorityNormal,
};

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); // (MISRA C 2004 rule 8.1)

__weak void PreSleepProcessing(uint32_t ulExpectedIdleTime)
{
// place for user code
}

__weak void PostSleepProcessing(uint32_t ulExpectedIdleTime)
{
// place for user code
}

// FreeRTOS initialization
void MX_FREERTOS_Init(void)
{

    // add mutexes, ...
    // add semaphores, ...
    // start timers, add new ones, ...
    // add queues, ...
    // Create the thread(s)
    // creation of defaultTask
    defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

    // add threads, ...
    // add events, ...
}

// Function implementing the defaultTask thread.
void StartDefaultTask(void *argument)
{
    // Infinite loop
    for(;;) {
        osDelay(1);
    }
}
