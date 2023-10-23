// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "main.h"
#include "cmsis_os.h"
#include "adc.h"
#include "lptim.h"
#include "rng.h"
#include "rtc.h"
#include "usb.h"
#include "wwdg.h"
#include "gpio.h"
#include "can.h"
#include "i2c.h"
#include "dma.h"
#include "usart.h"
#include "spi.h"

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
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
void MX_FREERTOS_Init(void);

// System entry point
int main(void)
{

    // Copy the vectors
    memcpy(vector_t, (uint8_t *) FLASH_BASE, sizeof(vector_t));
    SCB->VTOR = (uint32_t) vector_t;

    // Clear pending flash errors if any
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);

    // Reset of all peripherals, Initializes the Flash interface and the Systick.
    HAL_Init();

    // Configure the system clock
    SystemClock_Config();

    // Allow the timer to be enabled
    HAL_EnableTick();
    HAL_InitTick(TICK_INT_PRIORITY);
    TIMER_IF_Init();

    // Configure the peripherals common clocks
    PeriphCommonClock_Config();

    // Initialize all configured peripherals
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_DBG_Init();
////    MX_RTC_Init();
////    MX_LPTIM1_Init();
//    MX_RNG_Init();
//    MX_ADC1_Init();
//    MX_WWDG_Init();
//    MX_USB_PCD_Init();
//    MX_CAN1_Init();
//    MX_I2C1_Init();
//    MX_I2C3_Init();
//    MX_LPUART1_UART_Init(false, LPUART1_BAUDRATE);
//    MX_LPUART1_UART_Init(true, LPUART1_BAUDRATE);
//    MX_USART1_UART_Init(USART1_BAUDRATE);
//    MX_USART2_UART_Init(USART2_BAUDRATE);
//    MX_SPI1_Init();
//    MX_SPI2_Init();

    // Init scheduler
    osKernelInitialize();  // Call init function for freertos objects (in freertos.c)

    // Initialize the FreeRTOS app
    appInit();

    // Start scheduler
    osKernelStart();
    while (1) {}

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
    RCC_OscInitStruct.OscillatorType |= RCC_OSCILLATORTYPE_MSI;
    RCC_OscInitStruct.MSIState = RCC_MSI_ON;
    RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
    RCC_OscInitStruct.PLL.PLLM = 1;
    RCC_OscInitStruct.PLL.PLLN = 40;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
    RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
    RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    // Initializes the CPU, AHB and APB buses clocks
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
        Error_Handler();
    }

    // Enable MSI Auto calibration
    HAL_RCCEx_EnableMSIPLLMode();

    // Ensure that MSI is wake-up system clock
    __HAL_RCC_WAKEUPSTOP_CLK_CONFIG(RCC_STOP_WAKEUPCLOCK_MSI);

}

// Peripherals Common Clock Configuration
void PeriphCommonClock_Config(void)
{

    // Initializes the peripherals clock
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
    PeriphClkInit.PLLSAI1.PLLSAI1Source = RCC_PLLSOURCE_MSI;
    PeriphClkInit.PLLSAI1.PLLSAI1M = 1;
    PeriphClkInit.PLLSAI1.PLLSAI1N = 24;
    PeriphClkInit.PLLSAI1.PLLSAI1P = RCC_PLLP_DIV7;
    PeriphClkInit.PLLSAI1.PLLSAI1Q = RCC_PLLQ_DIV2;
    PeriphClkInit.PLLSAI1.PLLSAI1R = RCC_PLLR_DIV2;
    PeriphClkInit.PLLSAI1.PLLSAI1ClockOut = RCC_PLLSAI1_48M2CLK|RCC_PLLSAI1_ADC1CLK;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
        Error_Handler();
    }

}

// Period elapsed callback in non blocking mode
// This function is called  when TIM2 interrupt took place, inside
// HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
// a global variable "uwTick" used as application time base.
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2) {
        HAL_IncTick();
    }
}

// This function is executed in case of error occurrence.
void Error_Handler(void)
{
    // User can add his own implementation to report the HAL error return state
    __disable_irq();
    while (1) {
    }
}

#ifdef  USE_FULL_ASSERT
// Assertion trap
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif // USE_FULL_ASSERT

// Get the currently active peripherals
void MY_ActivePeripherals(char *buf, uint32_t buflen)
{
    *buf = '\0';
    if ((peripherals & PERIPHERAL_RNG) != 0) {
        strlcat(buf, "RNG ", buflen);
    }
    if ((peripherals & PERIPHERAL_USB) != 0) {
        strlcat(buf, "RNG ", buflen);
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
