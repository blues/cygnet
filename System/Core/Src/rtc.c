// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "rtc.h"

RTC_HandleTypeDef hrtc;

// RTC init function
void MX_RTC_Init(void)
{

    // Initialize RTC Only
    hrtc.Instance = RTC;
    hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
    hrtc.Init.AsynchPrediv = 127;
    hrtc.Init.SynchPrediv = 255;
    hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
    hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
    hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
    if (HAL_RTC_Init(&hrtc) != HAL_OK) {
        Error_Handler();
    }

}

// RTC msp init
void HAL_RTC_MspInit(RTC_HandleTypeDef* rtcHandle)
{

    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
    if(rtcHandle->Instance==RTC) {

        // Initializes the peripherals clock
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
        PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
            Error_Handler();
        }

        // RTC clock enable
        __HAL_RCC_RTC_ENABLE();

    }
}

// RTC msp deinit
void HAL_RTC_MspDeInit(RTC_HandleTypeDef* rtcHandle)
{

    if(rtcHandle->Instance==RTC) {

        // Peripheral clock disable
        __HAL_RCC_RTC_DISABLE();

    }
}
