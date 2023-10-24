// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "main.h"
#include "gpio.h"

// Configure GPIO
void MX_GPIO_Init(void)
{

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // GPIO Ports Clock Enable
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();

    // Default all pins to analog except SWD pins.  (This has a hard-wired
    // assumption that SWDIO_GPIO_Port and SWCLK_GPIO_Port are GPIOA.)
    memset(&GPIO_InitStruct, 0, sizeof(GPIO_InitStruct));
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Pin = GPIO_PIN_All & (~(SWDIO_Pin|SWCLK_Pin));
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = GPIO_PIN_All;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

    // Disable things that should be disabled to start
    MX_GPIO_Stop();

    // Configure USB detect input
#ifdef USB_DETECT_Pin
    memset(&GPIO_InitStruct, 0, sizeof(GPIO_InitStruct));
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pin = USB_DETECT_Pin;
    HAL_GPIO_Init(USB_DETECT_GPIO_Port, &GPIO_InitStruct);
    HAL_NVIC_SetPriority(USB_DETECT_EXTI_IRQn, INTERRUPT_PRIO_EXTI, 0);
    HAL_NVIC_EnableIRQ(USB_DETECT_EXTI_IRQn);
#endif

    // Configure button input
#if defined(USER_BTN_Pin)
    memset(&GPIO_InitStruct, 0, sizeof(GPIO_InitStruct));
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
#if	(USER_BTN_STATE_RELEASED == GPIO_PIN_SET)
    GPIO_InitStruct.Pull = GPIO_PULLUP;
#else
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
#endif
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pin = USER_BTN_Pin;
    HAL_GPIO_Init(USER_BTN_GPIO_Port, &GPIO_InitStruct);
    HAL_NVIC_SetPriority(USER_BTN_EXTI_IRQn, INTERRUPT_PRIO_EXTI, 0);
    HAL_NVIC_EnableIRQ(USER_BTN_EXTI_IRQn);
#endif

}

// Make sure that all the peripherals that are supposed to be powered-off before
// going into stop mode are in fact powered off.
void MX_GPIO_Stop()
{
    GPIO_InitTypeDef GPIO_InitStruct;

    memset(&GPIO_InitStruct, 0, sizeof(GPIO_InitStruct));
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    // LED
    GPIO_InitStruct.Pin = LED_BUILTIN_Pin;
    HAL_GPIO_Init(LED_BUILTIN_GPIO_Port, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LED_BUILTIN_GPIO_Port, LED_BUILTIN_Pin, GPIO_PIN_RESET);

}
