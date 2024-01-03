// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <string.h>
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

// global
size_t strlcpy(char *dst, const char *src, size_t siz);
size_t strlcat(char *dst, const char *src, size_t siz);

// main.c
int main(void);
void SystemClock_Config(void);
extern uint32_t peripherals;
void MX_ActivePeripherals(char *buf, uint32_t buflen);
uint32_t MX_ImageSize(void);
void MX_GetUniqueId(uint8_t *id);

// stm32l4xx_it.c
void Error_Handler(void);

// stm32l4xx_hal_timebase_tim.c
void HAL_SuspendTick(void);
void HAL_ResumeTick(void);
void HAL_IncTick(void);
uint32_t HAL_GetTick(void);
int64_t MX_GetTickMs(void);
void MX_StepTickMs(uint32_t msElapsed);
int64_t MX_GetTickMsFromISR(void);
int64_t MX_SteppedTickMs(void);

// app_weak.c
void appInit(void);
void appISR(uint16_t);
void appHeartbeatISR(uint32_t heartbeatSecs);
bool appSleepAllowed(void);
void appInitGPIO(void);
void appPreSleepProcessing(uint32_t ulExpectedIdleTime);
void appPostSleepProcessing(uint32_t ulExpectedIdleTime);

// debug_if.c
void MX_Breakpoint(void);
void MX_Restart(void);
bool MX_InISR(void);
#define MX_InterruptsDisabled() (__get_PRIMASK() != 0)
void MX_JumpToBootloader(void);
bool MX_DBG_Active(void);
void MX_DBG_Init(void);
void MX_DBG_SetOutput(void (*fn)(uint8_t *buf, uint32_t buflen));
void MX_DBG(const char *message, size_t length);
bool MX_DBG_Enable(bool on);

// heap_4.c
extern uint32_t heapPhysical;
extern uint32_t heapFreeAtStartup;

