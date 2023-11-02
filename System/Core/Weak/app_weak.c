// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "main.h"

// Initialize the app, creating tasks as needed
__weak void appInit(void)
{
}

// Return true if sleep is allowed
__weak bool appSleepAllowed(void)
{
    return false;
}

// Initialize the app's GPIOs
__weak void appInitGPIO(void)
{
}

// RTC heartbeat, used for timeouts and watchdogs
__weak void appHeartbeatISR(uint32_t heartbeatSecs)
{
}

// EXTI interrupt
__weak void appISR(uint16_t pins)
{
    (void) pins;
}

// See FreeRTOSConfig.h where this is registered via configPRE_SLEEP_PROCESSING()
__weak void appPreSleepProcessing(uint32_t ulExpectedIdleTime)
{
    (void) ulExpectedIdleTime;
}

// See FreeRTOSConfig.h where this is registered via configPOST_SLEEP_PROCESSING()
__weak void appPostSleepProcessing(uint32_t ulExpectedIdleTime)
{
    (void) ulExpectedIdleTime;
}
