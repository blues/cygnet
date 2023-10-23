// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "dma.h"

// Configure DMA by enabling all DMA clocks
void MX_DMA_Init(void)
{
    __HAL_RCC_DMA1_CLK_ENABLE();
    __HAL_RCC_DMA2_CLK_ENABLE();
}

// Disable DMA controller clock
void MX_DMA_DeInit(void)
{
    __HAL_RCC_DMA1_CLK_DISABLE();
    __HAL_RCC_DMA2_CLK_DISABLE();
}
