// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#pragma once

#include "main.h"
#include "stm32_lpm.h"

// stm32_lpm_if.c
extern const struct UTIL_LPM_Driver_s UTIL_PowerDriver;

// Enters Low Power Off Mode
void PWR_EnterOffMode(void);

// Exits Low Power Off Mode
void PWR_ExitOffMode(void);

// Enters Low Power Stop Mode
void PWR_EnterStopMode(void);

// Exits Low Power Stop Mode
void PWR_ExitStopMode(void);

// Enters Low Power Sleep Mode
void PWR_EnterSleepMode(void);

// Exits Low Power Sleep Mode
void PWR_ExitSleepMode(void);

