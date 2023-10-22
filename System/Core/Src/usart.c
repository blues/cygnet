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
void MX_LPUART1_UART_Init(bool altPins)
{
	lpuart1UsingAlternatePins = altPins;

    hlpuart1.Instance = LPUART1;
    hlpuart1.Init.BaudRate = 209700;
    hlpuart1.Init.WordLength = UART_WORDLENGTH_7B;
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
void MX_USART1_UART_Init(void)
{

    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
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
void MX_USART2_UART_Init(bool useRS485)
{
	usart2UsingRS485 = useRS485

    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
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

    if(uartHandle->Instance==LPUART1) {

        // Initializes the peripherals clock
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_LPUART1;
        PeriphClkInit.Lpuart1ClockSelection = RCC_LPUART1CLKSOURCE_PCLK1;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
            Error_Handler();
        }

        // LPUART1 clock enable
        __HAL_RCC_LPUART1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        //LPUART1 GPIO Configuration
		// OZZIE
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF8_LPUART1;
		if (lpuart1UsingAlternatePins) {
	        GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3;
	        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
		} else {
	        GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3;
	        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
		}

        // LPUART1 interrupt Init
        HAL_NVIC_SetPriority(LPUART1_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(LPUART1_IRQn);

    }

    if(uartHandle->Instance==USART1) {

        // Initializes the peripherals clock
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
        PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
            Error_Handler();
        }

        // USART1 clock enable
		MX_DMA_Init();
        __HAL_RCC_USART1_CLK_ENABLE();

        __HAL_RCC_GPIOA_CLK_ENABLE();
        //USART1 GPIO Configuration
        PA9     ------> USART1_TX
        PA10     ------> USART1_RX

        GPIO_InitStruct.Pin = TX_Pin|RX_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        // USART1 DMA Init
        // USART1_RX Init
        hdma_usart1_rx.Instance = DMA2_Channel7;
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
        hdma_usart1_tx.Instance = DMA2_Channel6;
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
        HAL_NVIC_SetPriority(USART1_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(USART1_IRQn);
	    HAL_NVIC_SetPriority(USART1_RX_DMA_IRQn, 5, 0);
	    HAL_NVIC_EnableIRQ(USART1_RX_DMA_IRQn);
	    HAL_NVIC_SetPriority(USART1_TX_DMA_IRQn, 5, 0);
	    HAL_NVIC_EnableIRQ(USART1_TX_DMA_IRQn);

    }

    if(uartHandle->Instance==USART2) {

        // Initializes the peripherals clock
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2;
        PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
            Error_Handler();
        }

        // USART2 clock enable
		MX_DMA_Init();
        __HAL_RCC_USART2_CLK_ENABLE();

        __HAL_RCC_GPIOA_CLK_ENABLE();
        //USART2 GPIO Configuration
		PA1		------> USART2_DE
        PA2     ------> USART2_TX
        PA3     ------> USART2_RX

		if (usart2UsingRS485) {
			// OZZIE
	        GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3;
	        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	        GPIO_InitStruct.Pull = GPIO_NOPULL;
	        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	        GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
	        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
		} else {
	        GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3;
	        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	        GPIO_InitStruct.Pull = GPIO_NOPULL;
	        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	        GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
	        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
		}

        // USART2 DMA Init
        // USART2_RX Init
        hdma_usart2_rx.Instance = DMA1_Channel6;
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
        hdma_usart2_tx.Instance = DMA1_Channel7;
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
        HAL_NVIC_SetPriority(USART2_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(USART2_IRQn);
	    HAL_NVIC_SetPriority(USART2_RX_DMA_Channel, 5, 0);
	    HAL_NVIC_EnableIRQ(USART2_RX_DMA_Channel);
	    HAL_NVIC_SetPriority(USART2_TX_DMA_Channel, 5, 0);
	    HAL_NVIC_EnableIRQ(USART2_TX_DMA_Channel);

    }

}

// UART msp deinit
void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

    if(uartHandle->Instance==LPUART1) {

        // Peripheral clock disable
        __HAL_RCC_LPUART1_CLK_DISABLE();

		// Deinit GPIOs
		// OZZIE
		if (lpuart1UsingAlternatePins) {
	        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2|GPIO_PIN_3);
		} else {
	        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2|GPIO_PIN_3);
		}

        // LPUART1 interrupt Deinit
        HAL_NVIC_DisableIRQ(LPUART1_IRQn);

    }

    if(uartHandle->Instance==USART1) {

        // Peripheral clock disable
        __HAL_RCC_USART1_CLK_DISABLE();

        //USART1 GPIO Configuration
        PA9     ------> USART1_TX
        PA10     ------> USART1_RX

        HAL_GPIO_DeInit(GPIOA, TX_Pin|RX_Pin);

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

        //USART2 GPIO Configuration
		// OZZIE
        PA1     ------> USART2_DE
        PA2     ------> USART2_TX
        PA3     ------> USART2_RX

		if (usart2UsingRS485) {
	        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);
		} else {
	        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2|GPIO_PIN_3);
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
