// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "main.h"
#include "global.h"
#include "dma.h"
#include "stm32l4xx_hal_sai.h"

#include "main.h"

SAI_HandleTypeDef hsai_BlockA1;
DMA_HandleTypeDef hdma_sai1_a;

// SAI1 Initialization Function
void MX_SAI1_Init(void)
{
// Note that this initialization requires not only that we are optimized
// for high speed, but that HSI is the PLL source and that PLLM is
// configured at /4.
#ifdef CLOCK_OPTIMIZE_FOR_HIGH_SPEED
    hsai_BlockA1.Instance = SAI1_Block_A;
    hsai_BlockA1.Init.Protocol = SAI_FREE_PROTOCOL;
    hsai_BlockA1.Init.AudioMode = SAI_MODEMASTER_RX;
    hsai_BlockA1.Init.DataSize = SAI_DATASIZE_8;
    hsai_BlockA1.Init.FirstBit = SAI_FIRSTBIT_MSB;
    hsai_BlockA1.Init.ClockStrobing = SAI_CLOCKSTROBING_RISINGEDGE;
    hsai_BlockA1.Init.Synchro = SAI_ASYNCHRONOUS;
    hsai_BlockA1.Init.OutputDrive = SAI_OUTPUTDRIVE_DISABLE;
    hsai_BlockA1.Init.NoDivider = SAI_MASTERDIVIDER_DISABLE;
    hsai_BlockA1.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_HF;
    hsai_BlockA1.Init.AudioFrequency = SAI_AUDIO_FREQUENCY_48K;
    hsai_BlockA1.Init.SynchroExt = SAI_SYNCEXT_DISABLE;
    hsai_BlockA1.Init.MonoStereoMode = SAI_MONOMODE;
    hsai_BlockA1.Init.CompandingMode = SAI_NOCOMPANDING;
    hsai_BlockA1.FrameInit.FrameLength = 16;
    hsai_BlockA1.FrameInit.ActiveFrameLength = 1;
    hsai_BlockA1.FrameInit.FSDefinition = SAI_FS_STARTFRAME;
    hsai_BlockA1.FrameInit.FSPolarity = SAI_FS_ACTIVE_LOW;
    hsai_BlockA1.FrameInit.FSOffset = SAI_FS_FIRSTBIT;
    hsai_BlockA1.SlotInit.FirstBitOffset = 0;
    hsai_BlockA1.SlotInit.SlotSize = SAI_SLOTSIZE_DATASIZE;
    hsai_BlockA1.SlotInit.SlotNumber = 2;
    hsai_BlockA1.SlotInit.SlotActive = 0x00000003;
    if (HAL_SAI_Init(&hsai_BlockA1) != HAL_OK) {
        Error_Handler();
    }
#else
    Error_Handler();
#endif
}

// SAI1 De-initialization Function
void MX_SAI1_DeInit(void)
{
    HAL_SAI_DeInit(&hsai_BlockA1);
}

void HAL_SAI_MspInit(SAI_HandleTypeDef* hsai)
{

    GPIO_InitTypeDef GPIO_InitStruct;
    if (hsai->Instance==SAI1_Block_A) {

#ifdef CLOCK_OPTIMIZE_FOR_HIGH_SPEED
        // This assumes that RCC is configured with a source of HSI (16Mhz)
        // and with a PLLM divider of 4, because we ultimately then divide
        // the clock down to get a PLLSAI1P of 3.047619Mhz, which is
        // within the datasheet-specified range of the IMP34D05 which
        // is fClk >= 1.2Mhz && fClk <= 3.25Mhz.
        RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_SAI1;
        PeriphClkInit.Sai1ClockSelection = RCC_SAI1CLKSOURCE_PLLSAI1;
        PeriphClkInit.PLLSAI1.PLLSAI1Source = RCC_PLLSOURCE_HSI;
        PeriphClkInit.PLLSAI1.PLLSAI1M = 4;
        PeriphClkInit.PLLSAI1.PLLSAI1N = 16;
        PeriphClkInit.PLLSAI1.PLLSAI1P = RCC_PLLP_DIV21;
        PeriphClkInit.PLLSAI1.PLLSAI1Q = RCC_PLLQ_DIV2;
        PeriphClkInit.PLLSAI1.PLLSAI1R = RCC_PLLR_DIV2;
        PeriphClkInit.PLLSAI1.PLLSAI1ClockOut = RCC_PLLSAI1_SAI1CLK;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
            Error_Handler();
        }
#else
        Error_Handler();
#endif

        __HAL_RCC_SAI1_CLK_ENABLE();
        HAL_NVIC_SetPriority(SAI1_IRQn, INTERRUPT_PRIO_SAI, 0);
        HAL_NVIC_EnableIRQ(SAI1_IRQn);

        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        GPIO_InitStruct.Alternate = SAI1_AF;
        GPIO_InitStruct.Pin = SAI1_SCK_Pin;
        HAL_GPIO_Init(SAI1_SCK_GPIO_Port, &GPIO_InitStruct);
        GPIO_InitStruct.Pin = SAI1_FS_Pin;
        HAL_GPIO_Init(SAI1_FS_GPIO_Port, &GPIO_InitStruct);
        GPIO_InitStruct.Pin = SAI1_SD_Pin;
        HAL_GPIO_Init(SAI1_SD_GPIO_Port, &GPIO_InitStruct);

        hdma_sai1_a.Instance = SAI1_DMA_Channel;
        hdma_sai1_a.Init.Request = DMA_REQUEST_1;
        hdma_sai1_a.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_sai1_a.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_sai1_a.Init.MemInc = DMA_MINC_ENABLE;
        hdma_sai1_a.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_sai1_a.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_sai1_a.Init.Mode = DMA_NORMAL;
        hdma_sai1_a.Init.Priority = DMA_PRIORITY_HIGH;
        HAL_DMA_Init(&hdma_sai1_a);

        __HAL_LINKDMA(hsai,hdmarx,hdma_sai1_a);
        __HAL_LINKDMA(hsai,hdmatx,hdma_sai1_a);

        HAL_NVIC_SetPriority(SAI1_DMA_IRQn, INTERRUPT_PRIO_SAI, 0);
        HAL_NVIC_EnableIRQ(SAI1_DMA_IRQn);

    }
}

void HAL_SAI_MspDeInit(SAI_HandleTypeDef* hsai)
{

    if (hsai->Instance==SAI1_Block_A) {

        __HAL_RCC_SAI1_CLK_DISABLE();

        HAL_NVIC_DisableIRQ(SAI1_IRQn);
        HAL_NVIC_DisableIRQ(SAI1_DMA_IRQn);

        HAL_GPIO_DeInit(SAI1_SCK_GPIO_Port, SAI1_SCK_Pin);
        HAL_GPIO_DeInit(SAI1_FS_GPIO_Port, SAI1_FS_Pin);
        HAL_GPIO_DeInit(SAI1_SD_GPIO_Port, SAI1_SD_Pin);

        HAL_DMA_DeInit(hsai->hdmarx);
        HAL_DMA_DeInit(hsai->hdmatx);

    }

}
