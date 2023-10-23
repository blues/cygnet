// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "lptim.h"

LPTIM_HandleTypeDef hlptim1;

// LPTIM1 init function
void MX_LPTIM1_Init(void)
{

    hlptim1.Instance = LPTIM1;
    hlptim1.Init.Clock.Source = LPTIM_CLOCKSOURCE_APBCLOCK_LPOSC;
    hlptim1.Init.Clock.Prescaler = LPTIM_PRESCALER_DIV1;
    hlptim1.Init.Trigger.Source = LPTIM_TRIGSOURCE_SOFTWARE;
    hlptim1.Init.OutputPolarity = LPTIM_OUTPUTPOLARITY_HIGH;
    hlptim1.Init.UpdateMode = LPTIM_UPDATE_IMMEDIATE;
    hlptim1.Init.CounterSource = LPTIM_COUNTERSOURCE_INTERNAL;
    hlptim1.Init.Input1Source = LPTIM_INPUT1SOURCE_GPIO;
    hlptim1.Init.Input2Source = LPTIM_INPUT2SOURCE_GPIO;
    if (HAL_LPTIM_Init(&hlptim1) != HAL_OK) {
        Error_Handler();
    }

}

// LPTIM msp init
void HAL_LPTIM_MspInit(LPTIM_HandleTypeDef* lptimHandle)
{

    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
    if(lptimHandle->Instance==LPTIM1) {

        // Initializes the peripherals clock
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_LPTIM1;
        PeriphClkInit.Lptim1ClockSelection = RCC_LPTIM1CLKSOURCE_PCLK;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
            Error_Handler();
        }

        // LPTIM1 clock enable
        __HAL_RCC_LPTIM1_CLK_ENABLE();

        // LPTIM1 interrupt Init
        HAL_NVIC_SetPriority(LPTIM1_IRQn, INTERRUPT_PRIO_TIMER, 0);
        HAL_NVIC_EnableIRQ(LPTIM1_IRQn);

    }
}

// LPTIM msp deinit
void HAL_LPTIM_MspDeInit(LPTIM_HandleTypeDef* lptimHandle)
{

    if(lptimHandle->Instance==LPTIM1) {

        // Peripheral clock disable
        __HAL_RCC_LPTIM1_CLK_DISABLE();

        // LPTIM1 interrupt Deinit
        HAL_NVIC_DisableIRQ(LPTIM1_IRQn);

    }
}
