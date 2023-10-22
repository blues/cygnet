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

}

// RNG msp init
void HAL_RNG_MspInit(RNG_HandleTypeDef* rngHandle)
{
    if(rngHandle->Instance==RNG) {

        // RNG clock enable
        __HAL_RCC_RNG_CLK_ENABLE();

    }
}

// RNG msp deinit
void HAL_RNG_MspDeInit(RNG_HandleTypeDef* rngHandle)
{
    if(rngHandle->Instance==RNG) {

        // Peripheral clock disable
        __HAL_RCC_RNG_CLK_DISABLE();

    }
}
