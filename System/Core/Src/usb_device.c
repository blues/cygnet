// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "main.h"
#include "usb_device.h"
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_cdc.h"
#include "usbd_cdc_if.h"

USBD_HandleTypeDef hUsbDeviceFS;
extern USBD_DescriptorsTypeDef FS_Desc;

// USB init
void MX_USB_DEVICE_Init(void)
{

    // Peripheral active
    peripherals |= PERIPHERAL_USB;

    // Init Device Library, add supported class and start the library.
    if (USBD_Init(&hUsbDeviceFS, &FS_Desc, DEVICE_FS) != USBD_OK) {
        Error_Handler();
    }
    if (USBD_RegisterClass(&hUsbDeviceFS, &USBD_CDC) != USBD_OK) {
        Error_Handler();
    }
    if (USBD_CDC_RegisterInterface(&hUsbDeviceFS, &USBD_Interface_fops_FS) != USBD_OK) {
        Error_Handler();
    }
    if (USBD_Start(&hUsbDeviceFS) != USBD_OK) {
        Error_Handler();
    }
}


// DeInit function
void MX_USB_DEVICE_DeInit(void)
{

    // Deinit Device Library, which stops it
    USBD_DeInit(&hUsbDeviceFS);

    // Disable pins
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin = USB_DM_Pin|USB_DP_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(USB_DM_GPIO_Port, &GPIO_InitStruct);

    // Peripheral inactive
    peripherals &= ~PERIPHERAL_USB;

}
