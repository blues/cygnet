// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "main.h"
#include "spi.h"
#include "dma.h"

SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi2;
DMA_HandleTypeDef hdma_spi2_rx;
DMA_HandleTypeDef hdma_spi2_tx;

// SPI1 init function
void MX_SPI1_Init(void)
{

    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_HARD_OUTPUT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 7;
    hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
    hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
    if (HAL_SPI_Init(&hspi1) != HAL_OK) {
        Error_Handler();
    }

    peripherals |= PERIPHERAL_SPI1;

}

// SPI1 Deinitialization
void MX_SPI1_DeInit(void)
{
    peripherals &= ~PERIPHERAL_SPI1;
    HAL_SPI_DeInit(&hspi1);
}

// SPI2 init function
void MX_SPI2_Init(void)
{

    hspi2.Instance = SPI2;
    hspi2.Init.Mode = SPI_MODE_MASTER;
    hspi2.Init.Direction = SPI_DIRECTION_2LINES;
    hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi2.Init.NSS = SPI_NSS_HARD_OUTPUT;
    hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi2.Init.CRCPolynomial = 7;
    hspi2.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
    hspi2.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
    if (HAL_SPI_Init(&hspi2) != HAL_OK) {
        Error_Handler();
    }

    peripherals |= PERIPHERAL_SPI2;

}

// SPI2 Deinitialization
void MX_SPI2_DeInit(void)
{
    peripherals &= ~PERIPHERAL_SPI2;
    HAL_SPI_DeInit(&hspi2);
}

// SPI msp init
void HAL_SPI_MspInit(SPI_HandleTypeDef* spiHandle)
{

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (spiHandle->Instance==SPI1) {

        // SPI1 clock enable
        __HAL_RCC_SPI1_CLK_ENABLE();

        // GPIO init
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = SPI1_AF;
        GPIO_InitStruct.Pin = SPI1_CK_Pin;
        HAL_GPIO_Init(SPI1_CK_GPIO_Port, &GPIO_InitStruct);
        GPIO_InitStruct.Pin = SPI1_MI_Pin;
        HAL_GPIO_Init(SPI1_MI_GPIO_Port, &GPIO_InitStruct);
        GPIO_InitStruct.Pin = SPI1_MO_Pin;
        HAL_GPIO_Init(SPI1_MO_GPIO_Port, &GPIO_InitStruct);
        GPIO_InitStruct.Pin = SPI1_CS_Pin;
        HAL_GPIO_Init(SPI1_CS_GPIO_Port, &GPIO_InitStruct);

        // SPI1 interrupt Init
        HAL_NVIC_SetPriority(SPI1_IRQn, INTERRUPT_PRIO_SPI, 0);
        HAL_NVIC_EnableIRQ(SPI1_IRQn);

    }

    if (spiHandle->Instance==SPI2) {

        // SPI2 clock enable
        MX_DMA_Init();
        __HAL_RCC_SPI2_CLK_ENABLE();

        // GPIO config
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = SPI2_AF;
        GPIO_InitStruct.Pin = SPI2_D10_CK_Pin;
        HAL_GPIO_Init(SPI2_D10_CK_GPIO_Port, &GPIO_InitStruct);
        GPIO_InitStruct.Pin = SPI2_D9_MI_Pin;
        HAL_GPIO_Init(SPI2_D9_MI_GPIO_Port, &GPIO_InitStruct);
        GPIO_InitStruct.Pin = SPI2_D12_MO_Pin;
        HAL_GPIO_Init(SPI2_D12_MO_GPIO_Port, &GPIO_InitStruct);
        GPIO_InitStruct.Pin = SPI2_D6_CS_Pin;
        HAL_GPIO_Init(SPI2_D6_CS_GPIO_Port, &GPIO_InitStruct);

        // SPI2_RX Init
        hdma_spi2_rx.Instance = SPI2_RX_DMA_Channel;
        hdma_spi2_rx.Init.Request = DMA_REQUEST_1;
        hdma_spi2_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_spi2_rx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_spi2_rx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_spi2_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_spi2_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_spi2_rx.Init.Mode = DMA_NORMAL;
        hdma_spi2_rx.Init.Priority = DMA_PRIORITY_LOW;
        if (HAL_DMA_Init(&hdma_spi2_rx) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(spiHandle,hdmarx,hdma_spi2_rx);

        // SPI2_TX Init
        hdma_spi2_tx.Instance = SPI2_TX_DMA_Channel;
        hdma_spi2_tx.Init.Request = DMA_REQUEST_1;
        hdma_spi2_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_spi2_tx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_spi2_tx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_spi2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_spi2_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_spi2_tx.Init.Mode = DMA_NORMAL;
        hdma_spi2_tx.Init.Priority = DMA_PRIORITY_LOW;
        if (HAL_DMA_Init(&hdma_spi2_tx) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(spiHandle,hdmatx,hdma_spi2_tx);

        // SPI2 interrupt Init
        HAL_NVIC_SetPriority(SPI2_IRQn, INTERRUPT_PRIO_SPI, 0);
        HAL_NVIC_EnableIRQ(SPI2_IRQn);
        HAL_NVIC_SetPriority(SPI2_RX_DMA_IRQn, INTERRUPT_PRIO_SPI, 0);
        HAL_NVIC_EnableIRQ(SPI2_RX_DMA_IRQn);
        HAL_NVIC_SetPriority(SPI2_TX_DMA_IRQn, INTERRUPT_PRIO_SPI, 0);
        HAL_NVIC_EnableIRQ(SPI2_TX_DMA_IRQn);

    }

}

// SPI msp deinit
void HAL_SPI_MspDeInit(SPI_HandleTypeDef* spiHandle)
{

    if(spiHandle->Instance==SPI1) {

        // Peripheral clock disable
        __HAL_RCC_SPI1_CLK_DISABLE();

        // GPIO DeInit
        HAL_GPIO_DeInit(SPI1_CK_GPIO_Port, SPI1_CK_Pin);
        HAL_GPIO_DeInit(SPI1_MI_GPIO_Port, SPI1_MI_Pin);
        HAL_GPIO_DeInit(SPI1_MO_GPIO_Port, SPI1_MO_Pin);
        HAL_GPIO_DeInit(SPI1_CS_GPIO_Port, SPI1_CS_Pin);

        // SPI1 interrupt Deinit
        HAL_NVIC_DisableIRQ(SPI1_IRQn);

    }

    if(spiHandle->Instance==SPI2) {

        // Peripheral clock disable
        __HAL_RCC_SPI2_CLK_DISABLE();

        // GPIO deinit
        HAL_GPIO_DeInit(SPI2_D10_CK_GPIO_Port, SPI2_D10_CK_Pin);
        HAL_GPIO_DeInit(SPI2_D9_MI_GPIO_Port, SPI2_D9_MI_Pin);
        HAL_GPIO_DeInit(SPI2_D12_MO_GPIO_Port, SPI2_D12_MO_Pin);
        HAL_GPIO_DeInit(SPI2_D6_CS_GPIO_Port, SPI2_D6_CS_Pin);

        // SPI2 DMA DeInit
        HAL_DMA_DeInit(spiHandle->hdmarx);
        HAL_DMA_DeInit(spiHandle->hdmatx);

        // SPI2 interrupt Deinit
        HAL_NVIC_DisableIRQ(SPI2_IRQn);
        HAL_NVIC_DisableIRQ(SPI2_RX_DMA_IRQn);
        HAL_NVIC_DisableIRQ(SPI2_TX_DMA_IRQn);

    }

}
