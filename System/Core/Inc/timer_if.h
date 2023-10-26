// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.


#pragma once

#include "stm32_timer.h"

// Init RTC hardware
// Returns status based on @ref UTIL_TIMER_Status_t
UTIL_TIMER_Status_t TIMER_IF_Init(void);

// Set the alarm
// Note: The alarm is set at timeout from timer Reference (TimerContext)
// Returns Status based on @ref UTIL_TIMER_Status_t
UTIL_TIMER_Status_t TIMER_IF_StartTimer(uint32_t timeout);

// Stop the Alarm
// Returns Status based on @ref UTIL_TIMER_Status_t
UTIL_TIMER_Status_t TIMER_IF_StopTimer(void);

// Set timer Reference (TimerContext)
// Returns  Timer Reference Value in  Ticks
uint32_t TIMER_IF_SetTimerContext(void);

// Get the RTC timer Reference
// Returns Timer Value in  Ticks
uint32_t TIMER_IF_GetTimerContext(void);

// Get the timer elapsed time since timer Reference (TimerContext) was set
// Returns RTC Elapsed time in ticks
uint32_t TIMER_IF_GetTimerElapsedTime(void);

// Get the timer value
// Returns RTC Timer value in ticks
uint32_t TIMER_IF_GetTimerValue(void);

// Return the minimum timeout in ticks the RTC is able to handle
// Returns minimum value for a timeout in ticks
uint32_t TIMER_IF_GetMinimumTimeout(void);

// Delay ms by polling RTC
void TIMER_IF_DelayMs(uint32_t delay);

// Converts time in ms to time in ticks
// Returns time in timer ticks
uint32_t TIMER_IF_Convert_ms2Tick(uint32_t timeMilliSec);

// Converts time in ticks to time in ms
// Returns time in timer milliseconds
uint32_t TIMER_IF_Convert_Tick2ms(uint32_t tick);

// Get rtc time
// Returns time seconds
uint32_t TIMER_IF_GetTime(uint16_t *subSeconds);
