// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#pragma once

#include "main.h"

// LOW POWER MANAGER

// Supported requester to the MCU Low Power Manager - can be increased up  to 32
// It lists a bit mapping of all user of the Low Power Manager
typedef enum {
    CFG_LPM_APPLI_Id,
    CFG_LPM_UART_TX_Id,
} CFG_LPM_Id_t;

