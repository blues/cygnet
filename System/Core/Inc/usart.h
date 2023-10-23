// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#pragma once

#include "main.h"

extern UART_HandleTypeDef hlpuart1;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

void MX_LPUART1_UART_Init(bool altPins, uint32_t baudRate);
void MX_USART1_UART_Init(uint32_t baudRate);
void MX_USART2_UART_Init(bool useRS485, uint32_t baudRate);

