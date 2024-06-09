// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "main.h"
#include "cmsis_os.h"
#include "adc.h"
#include "lptim.h"
#include "rng.h"
#include "rtc.h"
#include "wwdg.h"
#include "gpio.h"
#include "can.h"
#include "i2c.h"
#include "dma.h"
#include "usart.h"
#include "spi.h"
#include "timer_if.h"

// Peripherals that are currently active
uint32_t peripherals = 0;

// See reference manual RM0394.  The size is the last entry's address in Table 46 + sizeof(uint32_t).
// (Don't be confused into using the entry number rather than the address, because there are negative
// entry numbers. The highest address is the only accurate way of determining the actual table size.)
#define VECTOR_TABLE_SIZE_BYTES (0x0190+sizeof(uint32_t))
#if defined ( __ICCARM__ ) /* IAR Compiler */
#pragma data_alignment=512
#elif defined ( __CC_ARM ) /* ARM Compiler */
__align(512)
#elif defined ( __GNUC__ ) /* GCC Compiler */
__attribute__ ((aligned (512)))
#endif
uint8_t vector_t[VECTOR_TABLE_SIZE_BYTES];

// Linker-related symbols
#if defined( __ICCARM__ )   // IAR
extern void *ROM_CONTENT$$Limit;
#else                       // STM32CubeIDE (gcc)
extern void *_highest_used_rom;
#endif

// Forwards
void MX_FREERTOS_Init(void);

// System entry point
int main(void)
{

    // Copy the vectors
    memcpy(vector_t, (uint8_t *) FLASH_BASE, sizeof(vector_t));
    SCB->VTOR = (uint32_t) vector_t;

    // Clear pending flash errors if any
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);

    // For debugging, retrive image size
    MX_ImageSize();

    // Reset of all peripherals, Initializes the Flash interface and the Systick.
    HAL_Init();

    // Configure the system clock
    SystemClock_Config();

    // Start the HAL timer
    HAL_InitTick(TICK_INT_PRIORITY);

    // Start the RTC via the timer package
    TIMER_IF_Init();

    // Initialize all configured peripherals
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_DBG_Init();

    // Init scheduler
    osKernelInitialize();  // Call init function for freertos objects (in freertos.c)

    // Initialize the FreeRTOS app
    appInit();

    // Start scheduler
    osKernelStart();

    // Must never return here
    Error_Handler();

}

// System Clock Configuration
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    // Configure LSE Drive Capability
    HAL_PWR_EnableBkUpAccess();
    __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

    // Configure the main internal regulator output voltage
    if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK) {
        Error_Handler();
    }

    // Initializes the RCC Oscillators according to the specified parameters
    // in the RCC_OscInitTypeDef structure.
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE;
    RCC_OscInitStruct.LSEState = RCC_LSE_ON;
    RCC_OscInitStruct.OscillatorType |= RCC_OSCILLATORTYPE_MSI|RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.MSIState = RCC_MSI_ON;
    RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_11;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    // Initializes the CPU, AHB and APB buses clocks
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }

    // Enable MSI Auto calibration
    HAL_RCCEx_EnableMSIPLLMode();

    // Ensure that MSI is wake-up system clock
    __HAL_RCC_WAKEUPSTOP_CLK_CONFIG(RCC_STOP_WAKEUPCLOCK_MSI);

}

// Get the currently active peripherals
void MX_ActivePeripherals(char *buf, uint32_t buflen)
{
    *buf = '\0';
    if ((peripherals & PERIPHERAL_RNG) != 0) {
        strlcat(buf, "RNG ", buflen);
    }
    if ((peripherals & PERIPHERAL_USB) != 0) {
        strlcat(buf, "USB ", buflen);
    }
    if ((peripherals & PERIPHERAL_ADC1) != 0) {
        strlcat(buf, "ADC1 ", buflen);
    }
    if ((peripherals & PERIPHERAL_CAN1) != 0) {
        strlcat(buf, "CAN1 ", buflen);
    }
    if ((peripherals & PERIPHERAL_LPUART1) != 0) {
        strlcat(buf, "LPUART1 ", buflen);
    }
    if ((peripherals & PERIPHERAL_USART1) != 0) {
        strlcat(buf, "USART1 ", buflen);
    }
    if ((peripherals & PERIPHERAL_USART2) != 0) {
        strlcat(buf, "USART2 ", buflen);
    }
    if ((peripherals & PERIPHERAL_I2C1) != 0) {
        strlcat(buf, "I2C1 ", buflen);
    }
    if ((peripherals & PERIPHERAL_I2C3) != 0) {
        strlcat(buf, "I2C3 ", buflen);
    }
    if ((peripherals & PERIPHERAL_SPI1) != 0) {
        strlcat(buf, "SPI1 ", buflen);
    }
    if ((peripherals & PERIPHERAL_SPI2) != 0) {
        strlcat(buf, "SPI2 ", buflen);
    }

}

// Get the image size
uint32_t MX_ImageSize()
{
#if defined( __ICCARM__ )   // IAR
    return (uint32_t) (&ROM_CONTENT$$Limit) - FLASH_BASE;
#else
    return (uint32_t) (&_highest_used_rom) - FLASH_BASE;
#endif
}

// Get the board 64 bits unique ID
void MX_GetUniqueId(uint8_t *id)
{
    uint32_t ID_1_3_val = HAL_GetUIDw0() + HAL_GetUIDw2();
    uint32_t ID_2_val = HAL_GetUIDw1();
    id[7] = (ID_1_3_val) >> 24;
    id[6] = (ID_1_3_val) >> 16;
    id[5] = (ID_1_3_val) >> 8;
    id[4] = (ID_1_3_val);
    id[3] = (ID_2_val) >> 24;
    id[2] = (ID_2_val) >> 16;
    id[1] = (ID_2_val) >> 8;
    id[0] = (ID_2_val);
}
