// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "main.h"

// Global msp init
void HAL_MspInit(void)
{

    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    // System interrupt init
    // PendSV_IRQn interrupt configuration
    HAL_NVIC_SetPriority(PendSV_IRQn, 15, 0);

}
