// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#pragma once

#include "main.h"

extern RTC_HandleTypeDef hrtc;

void MX_RTC_Init(void);
void MX_RTC_DeInit(void);
void MX_RTC_ResetWakeupTimer(void);
int64_t MX_RTC_GetMs(void);
bool MX_RTC_GetErrors(uint32_t *retFailures, uint32_t *retResets);
void MX_RTC_GetDateTime(int *retYear, int *retMon1, int *retDay1, int *retHour0, int *retMin0, int *retSec0, int *retMs);
bool MX_RTC_SetDateTime(int year, int mon1, int day1, int hour0, int min0, int sec0);
