// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "main.h"
#include "rtc.h"

RTC_HandleTypeDef hrtc;

#define BASEYEAR 2000                   // Must end in 00 because of the chip's leap year computations

#define RTCWUT_SECS             4
uint32_t rtcwutCounter =		0;
uint32_t rtcwutClockrate =		32768;	// LSE Hz
uint32_t rtcwutDivisor =		16;     // RTC DIV16 resolution

// Whether or not we had an RTC error
bool needToResetRTC = false;

// Failures
uint32_t rtcGetFailures = 0;
uint32_t rtcGetResets = 0;

// RTC function to set date/time to a value that is valid so that our millisecond
// timer will function, even if the time is wrong.
void MX_RTC_Reset(void)
{
    RTC_TimeTypeDef sTime;
    sTime.Hours = 0;
    sTime.Minutes = 0;
    sTime.Seconds = 0;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;
    if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK) {
        Error_Handler();
    }
    RTC_DateTypeDef sDate;
    sDate.WeekDay = RTC_WEEKDAY_MONDAY;
    sDate.Month = RTC_MONTH_JANUARY;
    sDate.Date = 1;
    sDate.Year = (2020-BASEYEAR); // Valid according to MX_RTC_GetDateTime rules
    if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK) {
        Error_Handler();
    }
    needToResetRTC = false;
}

// RTC init function
void MX_RTC_Init(void)
{

    // Make sure that the RTC is fully uninitialized, or it will not initialize properly
    // because of a call to __HAL_RTC_IS_CALENDAR_INITIALIZED().  Note that we also
    // leave hrtc in an uninitialized (ready) state, which is important because
    // the DeInit could fail and leave it in a TIMEOUT state which would block the
    // subsequent Init().
    hrtc.Instance = RTC;
    HAL_RTC_DeInit(&hrtc);
    memset(&hrtc, 0, sizeof(hrtc));

    // Initialize RTC
    hrtc.Instance = RTC;
    hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
    hrtc.Init.AsynchPrediv = RTC_PREDIV_A;
    hrtc.Init.SynchPrediv = RTC_PREDIV_S;
    hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
    hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
    hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
    if (HAL_RTC_Init(&hrtc) != HAL_OK) {
        Error_Handler();
    }

    // Enable the Alarm A
    RTC_AlarmTypeDef sAlarm = {0};
    sAlarm.AlarmTime.SubSeconds = 0x0;
    sAlarm.AlarmMask = RTC_ALARMMASK_NONE;
    sAlarm.Alarm = RTC_ALARM_A;
    if (HAL_RTC_SetAlarm(&hrtc, &sAlarm, 0) != HAL_OK) {
        Error_Handler();
    }

    if (!MX_RTC_GetDateTime(NULL, NULL, NULL, NULL, NULL, NULL, NULL)) {
        needToResetRTC = true;
    }

    // Initialize RTC and set the Time and Date
    // Note that we need to set it validly so that MY_RTC_GetDateTime() always
    // succeeds, else the scheduler millisecond timer won't function
    if (needToResetRTC || (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0) != 0x32F2)) {
        MX_RTC_Reset();
        HAL_RTCEx_BKUPWrite(&hrtc,RTC_BKP_DR0,0x32F2);
    }

    // Reset the wakeup timer
    MX_RTC_ResetWakeupTimer();

}

// RTC deinit function
void MX_RTC_DeInit(void)
{
    HAL_RTC_DeInit(&hrtc);
}

// RTC msp init
void HAL_RTC_MspInit(RTC_HandleTypeDef* rtcHandle)
{

    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
    if (rtcHandle->Instance==RTC) {

        // Initializes the peripherals clock
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
        PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
            Error_Handler();
        }

        // RTC clock enable
        __HAL_RCC_RTC_ENABLE();
        __HAL_RCC_RTCAPB_CLK_ENABLE();

		// Interrupt init
        HAL_NVIC_SetPriority(RTC_Alarm_IRQn, INTERRUPT_PRIO_RTC, 0);
        HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);
        HAL_NVIC_SetPriority(RTC_WKUP_IRQn, INTERRUPT_PRIO_RTC, 0);
        HAL_NVIC_EnableIRQ(RTC_WKUP_IRQn);

    }
}

// RTC msp deinit
void HAL_RTC_MspDeInit(RTC_HandleTypeDef* rtcHandle)
{

    if(rtcHandle->Instance==RTC) {

        // Peripheral clock disable
        __HAL_RCC_RTC_DISABLE();
        __HAL_RCC_RTCAPB_CLK_DISABLE();

        // RTC interrupt Deinit
        HAL_NVIC_DisableIRQ(RTC_Alarm_IRQn);

    }

}

// Reset our heartbeat wakeup timer
void MX_RTC_ResetWakeupTimer()
{

    // Initialize the WakeUp Timer, noting that AN4759 says that
    // the desired reload timer can be computed as follows:
    // 0) know that LSE is 32768Hz
    // 1) pick a prescaler based on the granularity of the timebase
    //    that you want.  For DIV16 the resolution is LSEHz/16 = 2048
    // 2) Now, given that you want an interrupt every SECS seconds,
    //    compute the timer reload value with the formula:
    //    ((LSEHz/DIVxx)*SECS)-1
    //    or, for a 10 second periodic interrupt, it would be
    //    ((32768/16)*10)-1 = 20479
    rtcwutCounter = ((rtcwutClockrate * RTCWUT_SECS) / rtcwutDivisor) - 1;
    HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, rtcwutCounter, RTC_WAKEUPCLOCK_RTCCLK_DIV16);

}

// Wakeup event
void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
    appHeartbeatISR(RTCWUT_SECS);
}

// RTC SetDateTime function, returns true if succeeds
bool MX_RTC_SetDateTime(int year, int mon1, int day1, int hour0, int min0, int sec0)
{

    // Set the date/time
    RTC_TimeTypeDef time;
    RTC_DateTypeDef date;
    date.Year = year-BASEYEAR;
    date.Month = mon1;
    date.Date = day1;
    date.WeekDay = RTC_WEEKDAY_MONDAY;
    time.Hours = hour0;
    time.Minutes = min0;
    time.Seconds = sec0;
    time.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    time.StoreOperation = RTC_STOREOPERATION_RESET;
    if (HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_RTC_SetDate(&hrtc, &date, RTC_FORMAT_BIN) != HAL_OK) {
        Error_Handler();
    }

    // Reset the wakeup timer, which is occasionally affected by setting the date/time
    MX_RTC_ResetWakeupTimer();

    // Done
    return true;
}

// RTC GetMs function which is guaranteed never to go backwards
#define is_leap(year) (((year) % 4) == 0 && (((year) % 100) != 0 || ((year) % 400) == 0))
int64_t MX_RTC_GetMs()
{

    // Get the date/time and millisecond clock from the RTC
    int year, mon1, day1, hour0, min0, sec0, ms0;
    MX_RTC_GetDateTime(&year, &mon1, &day1, &hour0, &min0, &sec0, &ms0);

    // Set up for timegm()
    int tm_mon = mon1-1;
    int tm_mday = day1;
    int tm_hour = hour0;
    int tm_min = min0;
    int tm_sec = sec0;

    // Private timegm() function
    static const unsigned cumdays[2][12] = {
        {31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365},
        {31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366}
    };
    bool year_is_leap = is_leap(year);

    // Optimization for a very stupid algorithm
    int64_t res = 0;
    if (year >= 2020) {
        res = 18262;
        for (unsigned i = 2020; i < year; ++i) {
            res += is_leap(i) ? 366 : 365;
        }
    } else {
        for (unsigned i = 1970; i < year; ++i) {
            res += is_leap(i) ? 366 : 365;
        }
    }

    // Another stupid algorithm replaced with optimization
    if (tm_mon > 0) {
        res += cumdays[year_is_leap][tm_mon-1];
    }

    res += tm_mday - 1;
    res *= 24;
    res += tm_hour;
    res *= 60;
    res += tm_min;
    res *= 60;
    res += tm_sec;

    // Convert to milliseconds and exit
    int64_t retMs = ((1000*(int64_t)res) + ms0);
    return retMs;

}

// RTC GetDate debug function
bool MX_RTC_GetErrors(uint32_t *retFailures, uint32_t *retResets)
{
    *retFailures = rtcGetFailures;
    *retResets = rtcGetResets;
    return (rtcGetFailures > 0 || rtcGetResets > 0);
}

// RTC GetDate function, returns TRUE if succeeded
bool MX_RTC_GetDateTime(int *retYear, int *retMon1, int *retDay1, int *retHour0, int *retMin0, int *retSec0, int *retMs)
{
    RTC_TimeTypeDef time;
    RTC_DateTypeDef date = {0};
    bool haveValidDateTime = false;
    // 2021-04-25 As a harmless defensive measure, because time is so critical, retry
    for (int i=0; i<25; i++) {
        // Attempt to get the most reliable date that we can by synchronizing the registers.  Note that
        // we can't call WaitForSynchro because when timer interrupts are disabled HAL_GetTick() won't advance.
        if (MX_InterruptsDisabled()) {
            for (int i=0; i<3; i++) {
                HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
                HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
            }
        } else {
            HAL_RTC_WaitForSynchro(&hrtc);
            HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
            HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
            HAL_RTC_WaitForSynchro(&hrtc);
        }
        // GetDate MUST be called immediately after GetTime for consistency, according to HAL doc
        if (HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN) == HAL_OK) {
            if (HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN) == HAL_OK) {
                // This is the check that ensures that when the module is booted that the time is not valid
                if (date.Year+BASEYEAR < 2020 || date.Year+BASEYEAR > 2121) {
                    MX_RTC_Reset();
                    rtcGetResets++;
                } else {
                    haveValidDateTime = true;
                    break;
                }
            }
        } else {
            // Always do GetTime/GetDate as a pair, even if failure
            HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
        }
        rtcGetFailures++;
    }
    if (retYear != NULL) {
        *retYear = date.Year+BASEYEAR;
    }
    if (retMon1 != NULL) {
        *retMon1 = date.Month;
    }
    if (retDay1 != NULL) {
        *retDay1 = date.Date;
    }
    if (retHour0 != NULL) {
        *retHour0 = time.Hours;
    }
    if (retMin0 != NULL) {
        *retMin0 = time.Minutes;
    }
    if (retSec0 != NULL) {
        *retSec0 = time.Seconds;
    }
    if (retMs != NULL) {
        *retMs = (int) 1000 * (time.SecondFraction - time.SubSeconds) / (time.SecondFraction + 1);
    }
    return haveValidDateTime;
}
