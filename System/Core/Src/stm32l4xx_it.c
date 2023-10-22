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
    while (1) {
    }
}

// This function handles Hard fault interrupt.
void HardFault_Handler(void)
{
    while (1) {
    }
}

// This function handles Memory management fault.
void MemManage_Handler(void)
{
    while (1) {
    }
}

// This function handles Prefetch fault, memory access fault.
void BusFault_Handler(void)
{
    while (1) {
    }
}

// This function handles Undefined instruction or illegal state.
void UsageFault_Handler(void)
{
    while (1) {
    }
}

// This function handles Debug monitor.
void DebugMon_Handler(void)
{
}

///**************************************************************************
// STM32L4xx Peripheral Interrupt Handlers
// Add here the Interrupt Handlers for the used peripherals.
// For the available peripheral interrupt handler names,
// please refer to the startup file (startup_stm32l4xx.s).
///**************************************************************************

// This function handles TIM2 global interrupt.
void TIM2_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim2);
}

// This function handles LPTIM1 global interrupt.
void LPTIM1_IRQHandler(void)
{
    HAL_LPTIM_IRQHandler(&hlptim1);
}

// This function handles USB event interrupt through EXTI line 17.
void USB_IRQHandler(void)
{
    HAL_PCD_IRQHandler(&hpcd_USB_FS);
}

///**************************************************************************
// STM32L4xx Peripheral Interrupt Handlers
// Add here the Interrupt Handlers for the used peripherals.
// For the available peripheral interrupt handler names,
// please refer to the startup file (startup_stm32l4xx.s).
///**************************************************************************
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
// This function handles DMA1 channel4 global interrupt.
void DMA1_Channel4_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_spi2_rx);
}

// This function handles DMA1 channel5 global interrupt.
void DMA1_Channel5_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_spi2_tx);
}

// This function handles DMA1 channel6 global interrupt.
void DMA1_Channel6_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_usart2_rx);
}

// This function handles DMA1 channel7 global interrupt.
void DMA1_Channel7_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_usart2_tx);
}

// This function handles DMA2 channel6 global interrupt.
void DMA2_Channel6_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_usart1_tx);
}

// This function handles DMA2 channel7 global interrupt.
void DMA2_Channel7_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_usart1_rx);
}

// This function handles SPI1 global interrupt.
void SPI1_IRQHandler(void)
	HAL_SPI_IRQHandler(&hspi1);
}

// This function handles SPI2 global interrupt.
void SPI2_IRQHandler(void)
	HAL_SPI_IRQHandler(&hspi2);
}

