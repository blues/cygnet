// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#pragma once

#include "main.h"

extern ADC_HandleTypeDef hadc1;

uint32_t MX_ADC1_ReadChannel(uint32_t adcChannel, uint16_t gpioPin, GPIO_TypeDef *gpioPort);
uint16_t MX_ADC_GetMcuVoltageMv(void);
int16_t MX_ADC_GetMcuTemperatureC(void);
