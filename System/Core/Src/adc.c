// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "adc.h"

ADC_HandleTypeDef hadc1;

#ifdef OZZIE
// Change ADC1_Init to take a mask of all analog GPIOs, and then msp init/deinit
// them and also have read/write functions that uses their ranks etc.
#endif

// ADC1 init function
void MX_ADC1_Init(void)
{

    ADC_ChannelConfTypeDef sConfig = {0};

    // Common config
    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    hadc1.Init.LowPowerAutoWait = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
    hadc1.Init.OversamplingMode = DISABLE;
    if (HAL_ADC_Init(&hadc1) != HAL_OK) {
        Error_Handler();
    }

    // Configure Regular Channel
#ifdef OZZIE
    sConfig.Channel = ADC_CHANNEL_BAT_VOLTAGE;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;
    sConfig.SingleDiff = ADC_SINGLE_ENDED;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset = 0;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        Error_Handler();
    }
#endif

}

// ADC MSP init function
void HAL_ADC_MspInit(ADC_HandleTypeDef* adcHandle)
{

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if(adcHandle->Instance==ADC1) {

        // Initializes the peripherals clock
        RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
        PeriphClkInit.AdcClockSelection = RCC_ADCCLKSOURCE_PLLSAI1;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
            Error_Handler();
        }

        // ADC1 clock enable
        __HAL_RCC_ADC_CLK_ENABLE();

        // ADC1 GPIO Configuration
#ifdef OZZIE
        GPIO_InitStruct.Pin = A0_Pin|A1_Pin|A2_Pin|A3_Pin
                              |A4_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = BAT_VOLTAGE_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(BAT_VOLTAGE_GPIO_Port, &GPIO_InitStruct);
#endif

    }
}

// ADC MSP de-init function
void HAL_ADC_MspDeInit(ADC_HandleTypeDef* adcHandle)
{

    if(adcHandle->Instance==ADC1) {

        // Peripheral clock disable
        __HAL_RCC_ADC_CLK_DISABLE();

        // Deinit pins
#ifdef OZZIE
        HAL_GPIO_DeInit(GPIOA, A0_Pin|A1_Pin|A2_Pin|A3_Pin|A4_Pin);
        HAL_GPIO_DeInit(BAT_VOLTAGE_GPIO_Port, BAT_VOLTAGE_Pin);
#endif

    }
}
