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

void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
void MX_FREERTOS_Init(void);

// The application entry point.
int main(void)
{

    // Reset of all peripherals, Initializes the Flash interface and the Systick.
    HAL_Init();

    // Configure the system clock
    SystemClock_Config();

	// Configure the peripherals common clocks
    PeriphCommonClock_Config();

    // Initialize all configured peripherals
    MX_GPIO_Init();
    MX_RTC_Init();
    MX_RNG_Init();
    MX_LPTIM1_Init();
//    MX_ADC1_Init();
//    MX_WWDG_Init();
//    MX_USB_PCD_Init();
//    MX_CAN1_Init();
//    MX_I2C1_Init();
//    MX_I2C3_Init();
//    MX_LPUART1_UART_Init(false);
//    MX_LPUART1_UART_Init(true);
//    MX_DMA_Init();
//    MX_USART1_UART_Init();
//    MX_USART2_UART_Init();
//    MX_SPI1_Init();
//    MX_SPI2_Init();

    // Init scheduler
    osKernelInitialize();  // Call init function for freertos objects (in freertos.c)
    MX_FREERTOS_Init();

    // Start scheduler
    osKernelStart();

    // We should never get here as control is now taken by the scheduler
    while (1) {
    }

}

// System Clock Configuration
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    // Configure the main internal regulator output voltage
    if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK) {
        Error_Handler();
    }

    // Configure LSE Drive Capability
    HAL_PWR_EnableBkUpAccess();
    __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

    // Initializes the RCC Oscillators according to the specified parameters
	// in the RCC_OscInitTypeDef structure.
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_LSE
                                       |RCC_OSCILLATORTYPE_MSI;
    RCC_OscInitStruct.LSEState = RCC_LSE_ON;
    RCC_OscInitStruct.LSIState = RCC_LSI_ON;
    RCC_OscInitStruct.MSIState = RCC_MSI_ON;
    RCC_OscInitStruct.MSICalibrationValue = 0;
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
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                  |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
        Error_Handler();
    }

    // Enable MSI Auto calibration
    HAL_RCCEx_EnableMSIPLLMode();
}

// Peripherals Common Clock Configuration
void PeriphCommonClock_Config(void)
{
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    // Initializes the peripherals clock
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB|RCC_PERIPHCLK_RNG
                                         |RCC_PERIPHCLK_ADC;
    PeriphClkInit.AdcClockSelection = RCC_ADCCLKSOURCE_PLLSAI1;
    PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLLSAI1;
    PeriphClkInit.RngClockSelection = RCC_RNGCLKSOURCE_PLLSAI1;
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
// Reports the name of the source file and the source line number
// where the assert_param error has occurred.
void assert_failed(uint8_t *file, uint32_t line)
{
    // User can add his own implementation to report the file name and line number,
	// ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line)
}
#endif // USE_FULL_ASSERT
