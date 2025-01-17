// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#pragma once

#include "main.h"

extern UART_HandleTypeDef hlpuart1;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

bool MX_UART_IsDMA(UART_HandleTypeDef *huart);

void MX_UART_IDLE_IRQHandler(UART_HandleTypeDef *huart);

void MX_LPUART1_UART_Init(bool altPins, uint32_t baudRate);
void MX_LPUART1_UART_ReInit(void);
void MX_LPUART1_UART_DeInit(void);
void MX_LPUART1_UART_Suspend(void);
void MX_LPUART1_UART_Resume(void);
void MX_LPUART1_UART_Transmit(uint8_t *buf, uint32_t len, uint32_t timeoutMs);

void MX_USART1_UART_Init(uint32_t baudRate);
void MX_USART1_UART_ReInit(void);
void MX_USART1_UART_DeInit(void);
void MX_USART1_UART_Transmit(uint8_t *buf, uint32_t len, uint32_t timeoutMs);

void MX_USART2_UART_Init(bool useRS485, uint32_t baudRate);
void MX_USART2_UART_ReInit(void);
void MX_USART2_UART_DeInit(void);
void MX_USART2_UART_Transmit(uint8_t *buf, uint32_t len, uint32_t timeoutMs);

void MX_UART_RxStart(UART_HandleTypeDef *huart);
void MX_UART_RxConfigure(UART_HandleTypeDef *huart, uint8_t *rxbuf, uint16_t rxbuflen, void (*cb)(UART_HandleTypeDef *huart, bool error));
bool MX_UART_RxAvailable(UART_HandleTypeDef *huart);
uint8_t MX_UART_RxGet(UART_HandleTypeDef *huart);
void MX_UART_Transmit(UART_HandleTypeDef *huart, uint8_t *buf, uint32_t len, uint32_t timeoutMs);
bool MX_UART_TransmitFull(UART_HandleTypeDef *huart, uint8_t *buf, uint32_t len, uint32_t timeoutMs);

// Receive complete for USB serial device
void MX_USB_RxCplt(uint8_t* buf, uint32_t buflen);

