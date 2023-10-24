// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "main.h"
#include "stm32l4xx_it.h"

extern LPTIM_HandleTypeDef hlptim1;
extern PCD_HandleTypeDef hpcd_USB_FS;
extern TIM_HandleTypeDef htim2;
extern CAN_HandleTypeDef hcan1;
extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c3;
extern UART_HandleTypeDef hlpuart1;
extern UART_HandleTypeDef huart1;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart1_tx;
extern UART_HandleTypeDef huart2;
extern DMA_HandleTypeDef hdma_usart2_rx;
extern DMA_HandleTypeDef hdma_usart2_tx;
extern SPI_HandleTypeDef hspi1;
extern SPI_HandleTypeDef hspi2;
extern DMA_HandleTypeDef hdma_spi2_rx;
extern DMA_HandleTypeDef hdma_spi2_tx;

// This function handles Non maskable interrupt.
void NMI_Handler(void)
{
    MX_Breakpoint();
    NVIC_SystemReset();
}

// This function handles Hard fault interrupt.
void HardFault_Handler(void)
{
    MX_Breakpoint();
    NVIC_SystemReset();
}

// This function handles Memory management fault.
void MemManage_Handler(void)
{
    MX_Breakpoint();
    NVIC_SystemReset();
}

// This function handles Prefetch fault, memory access fault.
void BusFault_Handler(void)
{
    MX_Breakpoint();
    NVIC_SystemReset();
}

// This function handles Undefined instruction or illegal state.
void UsageFault_Handler(void)
{
    MX_Breakpoint();
}

// This function handles Debug monitor.
void DebugMon_Handler(void)
{
}

// This function is executed in case of error occurrence.
void Error_Handler(void)
{
    MX_Breakpoint();
    NVIC_SystemReset();
}

// Assertion trap
#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif

// Tamper alarm
void TAMP_STAMP_LSECSS_SSRU_IRQHandler(void)
{
    HAL_RTCEx_SSRUIRQHandler(&hrtc);
}

// This function handles TIM2 Global Interrupt
void TIM2_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim2);
}

// IRQ handler
void RNG_IRQHandler(void)
{
    HAL_RNG_IRQHandler(&hrng);
}

// This function handles TIM2 global interrupt.
void TIM2_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim2);
}

void RTC_Alarm_IRQHandler(void)
{
    HAL_RTC_AlarmIRQHandler(&hrtc);
}

void RTC_WKUP_IRQHandler(void)
{
    HAL_RTCEx_WakeUpTimerIRQHandler(&hrtc);
}

// This function handles LPTIM1 global interrupt.
void LPTIM1_IRQHandler(void)
{
    HAL_LPTIM_IRQHandler(&hlptim1);
}

// USB interrupt
void USB_IRQHandler(void)
{
    HAL_PCD_IRQHandler(&hpcd_USB_FS);
}

// This function handles CAN1 TX interrupt.
void CAN1_TX_IRQHandler(void)
{
    HAL_CAN_IRQHandler(&hcan1);
}

// This function handles CAN1 RX0 interrupt.
void CAN1_RX0_IRQHandler(void)
{
    HAL_CAN_IRQHandler(&hcan1);
}

// This function handles CAN1 RX1 interrupt.
void CAN1_RX1_IRQHandler(void)
{
    HAL_CAN_IRQHandler(&hcan1);
}

// This function handles CAN1 SCE interrupt.
void CAN1_SCE_IRQHandler(void)
{
    HAL_CAN_IRQHandler(&hcan1);
}

// This function handles I2C1 event interrupt.
void I2C1_EV_IRQHandler(void)
{
    HAL_I2C_EV_IRQHandler(&hi2c1);
}

// This function handles I2C1 error interrupt.
void I2C1_ER_IRQHandler(void)
{
    HAL_I2C_ER_IRQHandler(&hi2c1);
}

// This function handles I2C3 event interrupt.
void I2C3_EV_IRQHandler(void)
{
    HAL_I2C_EV_IRQHandler(&hi2c3);
}

// This function handles I2C3 error interrupt.
void I2C3_ER_IRQHandler(void)
{
    HAL_I2C_ER_IRQHandler(&hi2c3);
}

// This function handles LPUART1 global interrupt.
void LPUART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&hlpuart1);
}

// This function handles USART1 global interrupt.
void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart1);
}

// This function handles USART2 global interrupt.
void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart2);
}

// This function handles SPI1 global interrupt.
void SPI1_IRQHandler(void)
HAL_SPI_IRQHandler(&hspi1);
}

// This function handles SPI2 global interrupt.
void SPI2_IRQHandler(void)
HAL_SPI_IRQHandler(&hspi2);
}


// This function handles SPI2 global interrupt.
void SPI2_RX_DMA_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_spi2_rx);
}

// This function handles SPI2 global interrupt.
void SPI2_TX_DMA_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_spi2_tx);
}

// This function handles USART2 global interrupt.
void USART2_RX_DMA_Channel(void)
{
    HAL_DMA_IRQHandler(&hdma_usart2_rx);
}

// This function handles USART2 global interrupt.
void USART2_TX_DMA_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_usart2_tx);
}

// This function handles USART1 global interrupt.
void USART1_TX_DMA_Channel(void)
{
    HAL_DMA_IRQHandler(&hdma_usart1_tx);
}

// This function handles USART1 global interrupt.
void USART1_RX_DMA_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_usart1_rx);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    appISR(GPIO_Pin);
}

void GPIO_EXTI_IRQHandler(uint16_t GPIO_Pin)
{
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_Pin) != RESET) {
        uint16_t GPIO_Line = GPIO_Pin & EXTI->PR1;
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_Pin);
        HAL_GPIO_EXTI_Callback(GPIO_Line);
    }
}

void EXTI0_IRQHandler( void )
{
    GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}

void EXTI1_IRQHandler( void )
{
    GPIO_EXTI_IRQHandler(GPIO_PIN_1);
}

void EXTI2_IRQHandler( void )
{
    GPIO_EXTI_IRQHandler(GPIO_PIN_2);
}

void EXTI3_IRQHandler( void )
{
    GPIO_EXTI_IRQHandler(GPIO_PIN_3);
}

void EXTI4_IRQHandler( void )
{
    GPIO_EXTI_IRQHandler(GPIO_PIN_4);
}

void EXTI9_5_IRQHandler( void )
{
    GPIO_EXTI_IRQHandler(GPIO_PIN_9|GPIO_PIN_8|GPIO_PIN_7|GPIO_PIN_6|GPIO_PIN_5);
}

void EXTI15_10_IRQHandler( void )
{
    GPIO_EXTI_IRQHandler(GPIO_PIN_15|GPIO_PIN_14|GPIO_PIN_13|GPIO_PIN_12|GPIO_PIN_11|GPIO_PIN_10);
}
