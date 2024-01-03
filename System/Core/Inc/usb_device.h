// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#pragma once

#include "stm32l4xx.h"
#include "stm32l4xx_hal.h"
#include "usbd_def.h"

void MX_USB_DEVICE_Init(void);
void MX_USB_DEVICE_DeInit(void);
uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len);
