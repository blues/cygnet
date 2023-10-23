// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "rng.h"

RNG_HandleTypeDef hrng;

// RNG init function
void MX_RNG_Init(void)
{
    hrng.Instance = RNG;
    if (HAL_RNG_Init(&hrng) != HAL_OK) {
        Error_Handler();
    }
    peripherals |= PERIPHERAL_RNG;
}

// RNG msp init
void HAL_RNG_MspInit(RNG_HandleTypeDef* rngHandle)
{
    if(rngHandle->Instance==RNG) {

        // Initializes the peripherals clock
        RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RNG;
        PeriphClkInit.RngClockSelection = RCC_RNGCLKSOURCE_PLLSAI1;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
            Error_Handler();
        }

        // Enable peripheral clock
        __HAL_RCC_RNG_CLK_ENABLE();

    }
}

// RNG msp deinit
void HAL_RNG_MspDeInit(RNG_HandleTypeDef* rngHandle)
{
    peripherals &= ~PERIPHERAL_RNG;
    if(rngHandle->Instance==RNG) {
        __HAL_RCC_RNG_CLK_DISABLE();
    }
}

// Get a random number
uint32_t MX_RNG_Get()
{

    uint32_t random;
    while (HAL_RNG_GenerateRandomNumber(&hrng, &random) != HAL_OK) {
        HAL_Delay(1);
    }
    return random;
}
