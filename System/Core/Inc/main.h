// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#pragma once

#include "stm32l4xx_hal.h"
#include "board.h"

// Peripherals (please update MY_ActivePeripherals()
#define PERIPHERAL_RNG		0x00000001
#define PERIPHERAL_USB		0x00000002
#define PERIPHERAL_ADC1		0x00000004
#define PERIPHERAL_CAN1		0x00000008
#define PERIPHERAL_LPUART1  0x00000010
#define PERIPHERAL_USART1   0x00000020
#define PERIPHERAL_USART2   0x00000040
#define PERIPHERAL_I2C1     0x00000080
#define PERIPHERAL_I2C3     0x00000100
#define PERIPHERAL_SPI1     0x00000200
#define PERIPHERAL_SPI2     0x00000400
extern uint32_t peripherals;

// main.c
void Error_Handler(void);

// app_weak.c
void appInit(void);

// stm32_lpm_if.c
extern const struct UTIL_LPM_Driver_s UTIL_PowerDriver;

// debug_if.c
bool MX_DBG_Active(void);
void MX_DBG_Init(void);
void MX_DBG_SetOutput(UART_HandleTypeDef *huart, void (*fn)(UART_HandleTypeDef *huart, uint8_t *buf, uint32_t buflen));
void MX_DBG(const char *message, size_t length);
bool MX_DBG_Enable(bool on);
