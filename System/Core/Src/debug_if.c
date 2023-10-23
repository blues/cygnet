// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include <stdio.h>
#include "main.h"

// IAR-only method of writing to debug terminal without loading printf library
#if defined( __ICCARM__ )
#include "LowLevelIOInterface.h"
#endif

// Initially debug is not enabled
bool dbgEnabled = false;

// Debug state
STATIC UART_HandleTypeDef *dbgOutputPort = NULL;
STATIC void (*dbgOutputFn)(UART_HandleTypeDef *huart, uint8_t *buf, uint32_t buflen) = NULL;

// See if the debugger is active
bool MX_DBG_Active()
{
    return ((CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) != 0);
}

// Output a message to the console, a line at a time because
// the STM32CubeIDE doesn't recognize \n as doing an implicit
// carriage return.
void MX_DBG(const char *message, size_t length)
{

    // Exit if not enabled
    if (!dbgEnabled) {
        return;
    }

    // Output on the appropriate port
    if (dbgOutputFn != NULL) {
        dbgOutputFn(dbgOutputPort, (uint8_t *)message, length);
    }

    // On IAR only, output to debug console without the 7KB overhead of
    // printf("%.*s", (int) length, message)
    // DISABLED by default because this slows the system down significantly
#if 0
#if defined( __ICCARM__ )
    if (MX_DBG_Active()) {
        __dwrite(_LLIO_STDOUT, (const unsigned char *)message, length);
    }
#endif
#endif

}

// Set debug output function
void MX_DBG_SetOutput(UART_HandleTypeDef *huart, void (*fn)(UART_HandleTypeDef *huart, uint8_t *buf, uint32_t buflen))
{
    dbgOutputPort = huart;
    dbgOutputFn = fn;
}

// Enable/disable debug output
bool MX_DBG_Enable(bool on)
{
    bool prevState = dbgEnabled;
    dbgEnabled = on;
    return prevState;
}

// Init debugging
void MX_DBG_Init(void)
{

    // Enable or disable debug mode
    if (MX_DBG_Active()) {
// Not sure what the STM32L433 equivalent is - can't find it in RM0394 Table 47
//        LL_EXTI_EnableIT_32_63(LL_EXTI_LINE_46);    // STM32WL RM0453 Table 93 38.3.4
        HAL_DBGMCU_EnableDBGSleepMode();
        HAL_DBGMCU_EnableDBGStopMode();
        HAL_DBGMCU_EnableDBGStandbyMode();
    } else {
        GPIO_InitTypeDef GPIO_InitStruct = {0};
        GPIO_InitStruct.Mode   = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull   = GPIO_NOPULL;
        GPIO_InitStruct.Pin    = SWCLK_Pin;
        HAL_GPIO_Init(SWCLK_GPIO_Port, &GPIO_InitStruct);
        GPIO_InitStruct.Pin    = SWDIO_Pin;
        HAL_GPIO_Init(SWDIO_GPIO_Port, &GPIO_InitStruct);
        HAL_DBGMCU_DisableDBGSleepMode();
        HAL_DBGMCU_DisableDBGStopMode();
        HAL_DBGMCU_DisableDBGStandbyMode();
    }

}
