// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "dma.h"
#include <stdatomic.h>

atomic_int dmaInUse = 0;

// Configure DMA by enabling all DMA clocks
void MX_DMA_Init(void)
{
    if (atomic_fetch_add(&dmaInUse, 1) == 0) {
        __HAL_RCC_DMA1_CLK_ENABLE();
        __HAL_RCC_DMA2_CLK_ENABLE();
    }
}

// Disable DMA controller clock
void MX_DMA_DeInit(void)
{
    if (atomic_fetch_sub(&dmaInUse, 1) == 1) {
        __HAL_RCC_DMA1_CLK_DISABLE();
        __HAL_RCC_DMA2_CLK_DISABLE();
    }
}
