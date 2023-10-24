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

// main.c
int main(void);
extern uint32_t peripherals;
void MX_ActivePeripherals(char *buf, uint32_t buflen);
uint32_t MX_ImageSize(void);
void MX_GetUniqueId(uint8_t *id);

// stm32l4xx_it.c
void Error_Handler(void);

// stm32l4xx_hal_timebase_tim.c
void HAL_SuspendTick(void);
void HAL_ResumeTick(void);

// app_weak.c
void appInit(void);
void appHeartbeatISR(void);
void appISR(void);

// debug_if.c
void MX_Breakpoint(void);
bool MX_InISR(void);
void MX_JumpToBootloader(void);
bool MX_DBG_Active(void);
void MX_DBG_Init(void);
void MX_DBG_SetOutput(UART_HandleTypeDef *huart, void (*fn)(UART_HandleTypeDef *huart, uint8_t *buf, uint32_t buflen));
void MX_DBG(const char *message, size_t length);
bool MX_DBG_Enable(bool on);
