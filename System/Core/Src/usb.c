// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "usb.h"

PCD_HandleTypeDef hpcd_USB_FS;

// USB init function
void MX_USB_PCD_Init(void)
{

    hpcd_USB_FS.Instance = USB;
    hpcd_USB_FS.Init.dev_endpoints = 8;
    hpcd_USB_FS.Init.speed = PCD_SPEED_FULL;
    hpcd_USB_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
    hpcd_USB_FS.Init.Sof_enable = DISABLE;
    hpcd_USB_FS.Init.low_power_enable = DISABLE;
    hpcd_USB_FS.Init.lpm_enable = DISABLE;
    hpcd_USB_FS.Init.battery_charging_enable = DISABLE;
    if (HAL_PCD_Init(&hpcd_USB_FS) != HAL_OK) {
        Error_Handler();
    }

}

// USB msp init
void HAL_PCD_MspInit(PCD_HandleTypeDef* pcdHandle)
{

    if (pcdHandle->Instance==USB) {

        // Initializes the peripherals clock
        RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
        PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLLSAI1;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
            Error_Handler();
        }

        // USB clock enable
        __HAL_RCC_USB_CLK_ENABLE();

        // USB interrupt Init
        HAL_NVIC_SetPriority(USB_IRQn, INTERRUPT_PRIO_USB, 0);
        HAL_NVIC_EnableIRQ(USB_IRQn);

    }
}

// USB msp deinit
void HAL_PCD_MspDeInit(PCD_HandleTypeDef* pcdHandle)
{

    if (pcdHandle->Instance==USB) {

        // Peripheral clock disable
        __HAL_RCC_USB_CLK_DISABLE();

        // USB interrupt Deinit
        HAL_NVIC_DisableIRQ(USB_IRQn);

    }
}
