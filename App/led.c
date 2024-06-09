// Copyright 2024 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "app.h"
#include "main.h"

// Signal on the module's LED that we have restated
void ledRestartSignal()
{
    for (int i=10; i>-10; i-=1) {
        HAL_GPIO_WritePin(LED_BUILTIN_GPIO_Port, LED_BUILTIN_Pin, GPIO_PIN_SET);
        for (volatile int j=0; j<GMAX(i,0)+4; j++) for (volatile int k=0; k<40000; k++) ;
        HAL_GPIO_WritePin(LED_BUILTIN_GPIO_Port, LED_BUILTIN_Pin, GPIO_PIN_RESET);
        for (volatile int j=0; j<GMAX(i,0)+4; j++) for (volatile int k=0; k<30000; k++) ;
    }
    HAL_GPIO_WritePin(LED_BUILTIN_GPIO_Port, LED_BUILTIN_Pin, GPIO_PIN_SET);
}

// Signal BUSY on or off
void ledEnable(bool enable)
{
    HAL_GPIO_WritePin(LED_BUILTIN_GPIO_Port, LED_BUILTIN_Pin, enable ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

// Signal BUSY on or off
bool ledIsEnabled(void)
{
    return (HAL_GPIO_ReadPin(LED_BUILTIN_GPIO_Port, LED_BUILTIN_Pin) != GPIO_PIN_RESET);
}
