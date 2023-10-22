// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "wwdg.h"

WWDG_HandleTypeDef hwwdg;

// WWDG init function
void MX_WWDG_Init(void)
{

    hwwdg.Instance = WWDG;
    hwwdg.Init.Prescaler = WWDG_PRESCALER_1;
    hwwdg.Init.Window = 64;
    hwwdg.Init.Counter = 64;
    hwwdg.Init.EWIMode = WWDG_EWI_DISABLE;
    if (HAL_WWDG_Init(&hwwdg) != HAL_OK) {
        Error_Handler();
    }

}

// WWDG msp init
void HAL_WWDG_MspInit(WWDG_HandleTypeDef* wwdgHandle)
{

    if(wwdgHandle->Instance==WWDG) {

        // WWDG clock enable
        __HAL_RCC_WWDG_CLK_ENABLE();

    }
}
