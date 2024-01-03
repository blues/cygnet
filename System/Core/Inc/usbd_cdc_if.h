// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#pragma once

#include "usbd_cdc.h"

#define APP_RX_DATA_SIZE  1024
#define APP_TX_DATA_SIZE  1024

extern USBD_CDC_ItfTypeDef USBD_Interface_fops_FS;
