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
static UART_HandleTypeDef *dbgOutputPort = NULL;
static void (*dbgOutputFn)(UART_HandleTypeDef *huart, uint8_t *buf, uint32_t buflen) = NULL;

// For panic breakpoint
void MX_Breakpoint()
{
    if (MX_DBG_Active()) {
        asm ("BKPT 0");
    }
}

// For true fatal reboot
void MX_Restart()
{
    MX_Breakpoint();
    NVIC_SystemReset();
}

// See if in an ISR
bool MX_InISR()
{
    return ((SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0);
}

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

// Jump to the bootloader in system memory directly
void MX_JumpToBootloader()
{

    // Disable instruction cache
#ifdef HAL_ICACHE_MODULE_ENABLED
    if (HAL_ICACHE_IsEnabled() == 1) {
        (void)HAL_ICACHE_Disable();
    }
#endif

    // Hard-coded for this processor, and stolen/adapted by JW from:
    // http://www.keil.com/support/docs/3913.htm
    // https://stm32f4-discovery.net/2017/04/tutorial-jump-system-memory-software-stm32/

    // Disable SysTick and clear its exception pending bit, if it is used in the bootloader, e. g. by the RTX
    SysTick->LOAD = 0;
    SysTick->VAL = 0;
    SysTick->CTRL = 0;
    SCB->ICSR |= SCB_ICSR_PENDSTCLR_Msk ;

    // Disable interrupts.
    __disable_irq();

    // Disable all enabled interrupts in NVIC.
    for (int i=0; i<(sizeof(NVIC->ICER)/sizeof(NVIC->ICER[0])); i++) {
        NVIC->ICER[i] = 0xFFFFFFFF;
    }

    // Clear all pending interrupt requests in NVIC
    for (int i=0; i<(sizeof(NVIC->ICER)/sizeof(NVIC->ICPR[0])); i++) {
        NVIC->ICPR[i] = 0xFFFFFFFF;
    }

    // Disable individual fault handlers if the bootloader used them.
    SCB->SHCSR &= ~( SCB_SHCSR_USGFAULTENA_Msk |     \
                     SCB_SHCSR_BUSFAULTENA_Msk |     \
                     SCB_SHCSR_MEMFAULTENA_Msk ) ;

    // Remap system memory to address 0x00000000.
    __DSB();
    __ISB();
    __HAL_SYSCFG_REMAPMEMORY_SYSTEMFLASH();
    __DSB();
    __ISB();

    // Activate the MSP, if the core is found to currently run with the PSP.
    // As the compiler might still uses the stack, the PSP needs to be copied to the MSP before this.
    if (CONTROL_SPSEL_Msk & __get_CONTROL()) {
        // MSP is not active
        __set_MSP( __get_PSP( ) ) ;
        __set_CONTROL( __get_CONTROL( ) & ~CONTROL_SPSEL_Msk ) ;
    }

    // Load the vector table address of the user application into SCB->VTOR register.
    // Make sure the address meets the alignment requirements.
#define BOOTLOADER_BASE 0x1FFF0000
    SCB->VTOR = BOOTLOADER_BASE;

    // Initialize Stack Pointer
    __set_MSP(*(__IO uint32_t *) BOOTLOADER_BASE);

    // Jump to bootloader.
    typedef void (*pFunction)(void);
    pFunction jumpToBootloader = (pFunction) (*(uint32_t *)(BOOTLOADER_BASE + 4));
    jumpToBootloader();

    // We never get here.
    // At this point, the user may upload firmware to the STM32 in a variety of ways,
    // including connecting via USB and use the STM32CubeProgrammer application.

}
