// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "usart.h"

UART_HandleTypeDef hlpuart1;
bool lpuart1UsingAlternatePins = false;

UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart1_tx;

bool usart2UsingRS485 = false;
UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_rx;
DMA_HandleTypeDef hdma_usart2_tx;

// LPUART1 init function
void MX_LPUART1_UART_Init(bool altPins, uint32_t baudRate)
{
    lpuart1UsingAlternatePins = altPins;

    hlpuart1.Instance = LPUART1;
    hlpuart1.Init.BaudRate = baudRate;
    hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
    hlpuart1.Init.StopBits = UART_STOPBITS_1;
    hlpuart1.Init.Parity = UART_PARITY_NONE;
    hlpuart1.Init.Mode = UART_MODE_TX_RX;
    hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&hlpuart1) != HAL_OK) {
        Error_Handler();
    }

}

// USART1 init function
void MX_USART1_UART_Init(uint32_t baudRate)
{

    huart1.Instance = USART1;
    huart1.Init.BaudRate = baudRate;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&huart1) != HAL_OK) {
        Error_Handler();
    }

}

// USART2 init function
void MX_USART2_UART_Init(bool useRS485, uint32_t baudRate)
{
    usart2UsingRS485 = useRS485

	huart2.Instance = USART2;
    huart2.Init.BaudRate = baudRate;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&huart2) != HAL_OK) {
        Error_Handler();
    }

}

// UART msp init
void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    if (uartHandle->Instance==LPUART1) {

        // Initializes the peripherals clock
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_LPUART1;
        PeriphClkInit.Lpuart1ClockSelection = RCC_LPUART1CLKSOURCE_PCLK1;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
            Error_Handler();
        }

        // LPUART1 clock enable
        __HAL_RCC_LPUART1_CLK_ENABLE();

        // GPIO Configuration
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = LPUART1_AF;
        if (!lpuart1UsingAlternatePins) {
            GPIO_InitStruct.Pin = LPUART1_VCP_RX_Pin;
            HAL_GPIO_Init(LPUART1_VCP_RX_GPIO_Port, &GPIO_InitStruct);
            GPIO_InitStruct.Pin = LPUART1_VCP_TX_Pin;
            HAL_GPIO_Init(LPUART1_VCP_TX_GPIO_Port, &GPIO_InitStruct);
        } else {
            GPIO_InitStruct.Pin = LPUART1_A3_RX_Pin;
            HAL_GPIO_Init(LPUART1_A3_RX_GPIO_Port, &GPIO_InitStruct);
            GPIO_InitStruct.Pin = LPUART1_A2_TX_Pin;
            HAL_GPIO_Init(LPUART1_A2_TX_GPIO_Port, &GPIO_InitStruct);
        }

        // LPUART1 interrupt Init
        HAL_NVIC_SetPriority(LPUART1_IRQn, INTERRUPT_PRIO_SERIAL, 0);
        HAL_NVIC_EnableIRQ(LPUART1_IRQn);

    }

    if (uartHandle->Instance==USART1) {

        // Initializes the peripherals clock
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
        PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
            Error_Handler();
        }

        // USART1 clock enable
        MX_DMA_Init();
        __HAL_RCC_USART1_CLK_ENABLE();

        // GPIO Configuration
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = USART1_AF;
        GPIO_InitStruct.Pin = USART1_TX_Pin
		HAL_GPIO_Init(USART1_TX_GPIO_Port, &GPIO_InitStruct);
        GPIO_InitStruct.Pin = USART1_RX_Pin
		HAL_GPIO_Init(USART1_RX_GPIO_Port, &GPIO_InitStruct);

        // USART1_RX Init
        hdma_usart1_rx.Instance = USART1_RX_DMA_Channel;
        hdma_usart1_rx.Init.Request = DMA_REQUEST_2;
        hdma_usart1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_usart1_rx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_usart1_rx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_usart1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_usart1_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_usart1_rx.Init.Mode = DMA_NORMAL;
        hdma_usart1_rx.Init.Priority = DMA_PRIORITY_LOW;
        if (HAL_DMA_Init(&hdma_usart1_rx) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(uartHandle,hdmarx,hdma_usart1_rx);

        // USART1_TX Init
        hdma_usart1_tx.Instance = USART1_TX_DMA_Channel;
        hdma_usart1_tx.Init.Request = DMA_REQUEST_2;
        hdma_usart1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_usart1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_usart1_tx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_usart1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_usart1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_usart1_tx.Init.Mode = DMA_NORMAL;
        hdma_usart1_tx.Init.Priority = DMA_PRIORITY_LOW;
        if (HAL_DMA_Init(&hdma_usart1_tx) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(uartHandle,hdmatx,hdma_usart1_tx);

        // USART1 interrupt Init
        HAL_NVIC_SetPriority(USART1_IRQn, INTERRUPT_PRIO_SERIAL, 0);
        HAL_NVIC_EnableIRQ(USART1_IRQn);
        HAL_NVIC_SetPriority(USART1_RX_DMA_IRQn, INTERRUPT_PRIO_SERIAL, 0);
        HAL_NVIC_EnableIRQ(USART1_RX_DMA_IRQn);
        HAL_NVIC_SetPriority(USART1_TX_DMA_IRQn, INTERRUPT_PRIO_SERIAL, 0);
        HAL_NVIC_EnableIRQ(USART1_TX_DMA_IRQn);

    }

    if (uartHandle->Instance==USART2) {

        // Initializes the peripherals clock
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2;
        PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
            Error_Handler();
        }

        // USART2 clock enable
        MX_DMA_Init();
        __HAL_RCC_USART2_CLK_ENABLE();

        // GPIO Configuration
        if (!usart2UsingRS485) {
            GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
            GPIO_InitStruct.Pull = GPIO_NOPULL;
            GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
            GPIO_InitStruct.Alternate = USART2_AF;
            GPIO_InitStruct.Pin = USART2_A2_TX_Pin;
            HAL_GPIO_Init(USART2_A2_TX_GPIO_Port, &GPIO_InitStruct);
            GPIO_InitStruct.Pin = USART2_A2_RX_Pin;
            HAL_GPIO_Init(USART2_A2_RX_GPIO_Port, &GPIO_InitStruct);
        } else {
            GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
            GPIO_InitStruct.Pull = GPIO_NOPULL;
            GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
            GPIO_InitStruct.Alternate = RS485_AF;
            GPIO_InitStruct.Pin = RS485_A2_TX_Pin;
            HAL_GPIO_Init(RS485_A2_TX_GPIO_Port, &GPIO_InitStruct);
            GPIO_InitStruct.Pin = RS485_A2_RX_Pin;
            HAL_GPIO_Init(RS485_A2_RX_GPIO_Port, &GPIO_InitStruct);
            GPIO_InitStruct.Pin = RS485_12_DE_Pin;
            HAL_GPIO_Init(RS485_A1_DE_GPIO_Port, &GPIO_InitStruct);
        }

        // USART2_RX Init
        hdma_usart2_rx.Instance = USART2_RX_DMA_Channel;
        hdma_usart2_rx.Init.Request = DMA_REQUEST_2;
        hdma_usart2_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_usart2_rx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_usart2_rx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_usart2_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_usart2_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_usart2_rx.Init.Mode = DMA_NORMAL;
        hdma_usart2_rx.Init.Priority = DMA_PRIORITY_LOW;
        if (HAL_DMA_Init(&hdma_usart2_rx) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(uartHandle,hdmarx,hdma_usart2_rx);

        // USART2_TX Init
        hdma_usart2_tx.Instance = USART2_TX_DMA_Channel;
        hdma_usart2_tx.Init.Request = DMA_REQUEST_2;
        hdma_usart2_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_usart2_tx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_usart2_tx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_usart2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_usart2_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_usart2_tx.Init.Mode = DMA_NORMAL;
        hdma_usart2_tx.Init.Priority = DMA_PRIORITY_LOW;
        if (HAL_DMA_Init(&hdma_usart2_tx) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(uartHandle,hdmatx,hdma_usart2_tx);

        // USART2 interrupt Init
        HAL_NVIC_SetPriority(USART2_IRQn, INTERRUPT_PRIO_SERIAL, 0);
        HAL_NVIC_EnableIRQ(USART2_IRQn);
        HAL_NVIC_SetPriority(USART2_RX_DMA_Channel, INTERRUPT_PRIO_SERIAL, 0);
        HAL_NVIC_EnableIRQ(USART2_RX_DMA_Channel);
        HAL_NVIC_SetPriority(USART2_TX_DMA_Channel, INTERRUPT_PRIO_SERIAL, 0);
        HAL_NVIC_EnableIRQ(USART2_TX_DMA_Channel);

    }

}

// UART msp deinit
void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

    if (uartHandle->Instance==LPUART1) {

        // Peripheral clock disable
        __HAL_RCC_LPUART1_CLK_DISABLE();

        // Deinit GPIOs
        if (!lpuart1UsingAlternatePins) {
            HAL_GPIO_DeInit(LPUART1_VCP_RX_GPIO_Port, LPUART1_VCP_RX_Pin);
            HAL_GPIO_DeInit(LPUART1_VCP_TX_GPIO_Port, LPUART1_VCP_TX_Pin);
        } else {
            HAL_GPIO_DeInit(LPUART1_A3_RX_GPIO_Port, LPUART1_A3_RX_Pin);
            HAL_GPIO_DeInit(LPUART1_A2_TX_GPIO_Port, LPUART1_A2_TX_Pin);
        }

        // LPUART1 interrupt Deinit
        HAL_NVIC_DisableIRQ(LPUART1_IRQn);

    }

    if (uartHandle->Instance==USART1) {

        // Peripheral clock disable
        __HAL_RCC_USART1_CLK_DISABLE();

        //  GPIO Configuration
        HAL_GPIO_DeInit(USART1_TX_GPIO_Port, USART1_TX_Pin);
        HAL_GPIO_DeInit(USART1_RX_GPIO_Port, USART1_RX_Pin);

        // USART1 DMA DeInit
        HAL_DMA_DeInit(uartHandle->hdmarx);
        HAL_DMA_DeInit(uartHandle->hdmatx);

        // USART1 interrupt Deinit
        HAL_NVIC_DisableIRQ(USART1_IRQn);
        HAL_NVIC_DisableIRQ(USART1_RX_DMA_IRQn);
        HAL_NVIC_DisableIRQ(USART1_TX_DMA_IRQn);

    }

    if(uartHandle->Instance==USART2) {

        // Peripheral clock disable
        __HAL_RCC_USART2_CLK_DISABLE();

        // GPIO Configuration
        if (!usart2UsingRS485) {
            HAL_GPIO_DeInit(USART2_A2_TX_GPIO_Port, USART2_A2_TX_Pin);
            HAL_GPIO_DeInit(USART2_A2_RX_GPIO_Port, USART2_A2_RX_Pin);
        } else {
            HAL_GPIO_DeInit(RS485_A2_TX_GPIO_Port, RS485_A2_TX_Pin);
            HAL_GPIO_DeInit(RS485_A2_RX_GPIO_Port, RS485_A2_RX_Pin);
            HAL_GPIO_DeInit(RS485_A1_DE_GPIO_Port, RS485_12_DE_Pin);
        }

        // USART2 DMA DeInit
        HAL_DMA_DeInit(uartHandle->hdmarx);
        HAL_DMA_DeInit(uartHandle->hdmatx);

        // USART2 interrupt Deinit
        HAL_NVIC_DisableIRQ(USART2_IRQn);
        HAL_NVIC_DisableIRQ(USART2_RX_DMA_Channel);
        HAL_NVIC_DisableIRQ(USART2_TX_DMA_Channel);

    }

}
