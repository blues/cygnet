// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "main.h"
#include "rtc.h"

RTC_HandleTypeDef hrtc;

#define RTCWUT_SECS             4
uint32_t rtcwutCounter =		0;
uint32_t rtcwutClockrate =		32768;	// LSE Hz
uint32_t rtcwutDivisor =		16;     // RTC DIV16 resolution

// RTC init function
void MX_RTC_Init(void)
{
    RTC_AlarmTypeDef sAlarm = {0};

    // Initialize RTC Only
    hrtc.Instance = RTC;
    hrtc.Init.AsynchPrediv = RTC_PREDIV_A;
    hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
    hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
    hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
    if (HAL_RTC_Init(&hrtc) != HAL_OK) {
        Error_Handler();
    }

    // Enable the Alarm A
    sAlarm.AlarmTime.SubSeconds = 0x0;
    sAlarm.AlarmMask = RTC_ALARMMASK_NONE;
    sAlarm.Alarm = RTC_ALARM_A;
    if (HAL_RTC_SetAlarm(&hrtc, &sAlarm, 0) != HAL_OK) {
        Error_Handler();
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
    appHeartbeatISR();
}
