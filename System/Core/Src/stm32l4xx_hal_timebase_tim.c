// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_tim.h"
#include "main.h"

// Timer handle
TIM_HandleTypeDef        htim2;

// 1ms ticks
#define TICKS_PER_SECOND 1000
#define MILLISECONDS_PER_TICK (1000/TICKS_PER_SECOND)
__IO uint32_t tickCount;
__IO int64_t msCountFromTicks = 0;
__IO int64_t msCountFromSleep = 0;

// This function configures the TIM2 as a time base source.
// The time source is configured  to have 1ms time base with a dedicated
// Tick interrupt priority.
// This function is called  automatically at the beginning of program after
// reset by HAL_Init() or at any time when clock is configured, by HAL_RCC_ClockConfig().
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    RCC_ClkInitTypeDef    clkconfig;
    uint32_t              uwTimclock, uwAPB1Prescaler;

    uint32_t              uwPrescalerValue;
    uint32_t              pFLatency;
    HAL_StatusTypeDef     status = HAL_OK;

    // Enable TIM2 clock
    __HAL_RCC_TIM2_CLK_ENABLE();

    // Get clock configuration
    HAL_RCC_GetClockConfig(&clkconfig, &pFLatency);

    // Get APB1 prescaler
    uwAPB1Prescaler = clkconfig.APB1CLKDivider;

    // Compute TIM2 clock
    if (uwAPB1Prescaler == RCC_HCLK_DIV1) {
        uwTimclock = HAL_RCC_GetPCLK1Freq();
    } else {
        uwTimclock = 2UL * HAL_RCC_GetPCLK1Freq();
    }

    // Compute the prescaler value to have TIM2 counter clock equal to 1MHz
    uwPrescalerValue = (uint32_t) ((uwTimclock / 1000000U) - 1U);

    // Initialize TIM2
    htim2.Instance = TIM2;

    // Initialize TIMx peripheral as follow:
    // + Period = [(TIM2CLK/1000) - 1]. to have a (1/1000) s time base.
    // + Prescaler = (uwTimclock/1000000 - 1) to have a 1MHz counter clock.
    // + ClockDivision = 0
    // + Counter direction = Up
    htim2.Init.Period = (1000000U / 1000U) - 1U;
    htim2.Init.Prescaler = uwPrescalerValue;
    htim2.Init.ClockDivision = 0;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    status = HAL_TIM_Base_Init(&htim2);
    if (status == HAL_OK) {
        // Start the TIM time Base generation in interrupt mode
        status = HAL_TIM_Base_Start_IT(&htim2);
        if (status == HAL_OK) {
            // Enable the TIM2 global Interrupt
            HAL_NVIC_EnableIRQ(TIM2_IRQn);
            // Configure the SysTick IRQ priority
            if (TickPriority < (1UL << __NVIC_PRIO_BITS)) {
                // Configure the TIM IRQ priority
                HAL_NVIC_SetPriority(TIM2_IRQn, TickPriority, 0U);
                uwTickPrio = TickPriority;
            } else {
                status = HAL_ERROR;
            }
        }
    }

    // Return function status
    return status;
}

// Deinitialize the clock
HAL_StatusTypeDef HAL_DeInitTick()
{

    // Stop the interrupt
    HAL_TIM_Base_Stop_IT(&htim2);

    // Deinit the timer
    HAL_TIM_Base_DeInit(&htim2);

    // Disable TIM2 clock
    __HAL_RCC_TIM2_CLK_DISABLE();

    // Disable the TIM2 global Interrupt
    HAL_NVIC_DisableIRQ(TIM2_IRQn);

    return HAL_OK;
}

// Suspend Tick increment.
// Disable the tick increment by disabling TIM2 update interrupt.
void HAL_SuspendTick(void)
{
    // Disable TIM2 update Interrupt
    __HAL_TIM_DISABLE_IT(&htim2, TIM_IT_UPDATE);
}

// Resume Tick increment.
// Enable the tick increment by Enabling TIM2 update interrupt.
void HAL_ResumeTick(void)
{
    // Enable TIM2 Update in
    __HAL_TIM_ENABLE_IT(&htim2, TIM_IT_UPDATE);
}

// This is an auxiliary timer tick that occurs, helping us to make sure that we increment
// the main tick counter on a regular basis even though the main tick counter stops when
// we're in STOP2 mode.
void MX_StepTickMs(uint32_t msElapsed)
{
    msCountFromSleep += msElapsed;
}

// Return the total stepped ticks
int64_t MX_SteppedTickMs()
{
    return msCountFromSleep;
}

// Bump the tick counts
void HAL_IncTick(void)
{
    tickCount++;
    msCountFromTicks++;
}

// Delay milliseconds in a compute loop
void HAL_Delay(__IO uint32_t delayMs)
{
    int64_t expiresMs = MX_GetTickMs() + delayMs;
    while (MX_GetTickMs() < expiresMs) {
        __NOP();
    }
}

// Provide a tick value in milliseconds which pauses during STOP2 and which wraps
uint32_t HAL_GetTick(void)
{
    return tickCount * MILLISECONDS_PER_TICK;
}

// Provide an approximate tick value in milliseconds with the illusion that it is continuous, across STOP2.
int64_t MX_GetTickMs(void)
{
    int64_t ticks = msCountFromTicks;
    return ticks + msCountFromSleep;
}
