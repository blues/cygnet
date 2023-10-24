// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#pragma once

#include "main.h"

extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c3;

void MX_I2C1_Init(void);
void MX_I2C1_DeInit(void);
bool MY_I2C1_ReadRegister(uint16_t i2cAddress, uint8_t Reg, void *data, uint16_t maxdatalen, uint32_t timeoutMs);
bool MY_I2C1_WriteRegister(uint16_t i2cAddress, uint8_t Reg, void *data, uint16_t datalen, uint32_t timeoutMs);
bool MY_I2C1_Transmit(uint16_t i2cAddress, void *data, uint16_t datalen, uint32_t timeoutMs);
bool MY_I2C1_Receive(uint16_t i2cAddress, void *data, uint16_t maxdatalen, uint32_t timeoutMs);

void MX_I2C3_Init(void);
void MX_I2C3_DeInit(void);
bool MY_I2C3_ReadRegister(uint16_t i2cAddress, uint8_t Reg, void *data, uint16_t maxdatalen, uint32_t timeoutMs);
bool MY_I2C3_WriteRegister(uint16_t i2cAddress, uint8_t Reg, void *data, uint16_t datalen, uint32_t timeoutMs);
bool MY_I2C3_Transmit(uint16_t i2cAddress, void *data, uint16_t datalen, uint32_t timeoutMs);
bool MY_I2C3_Receive(uint16_t i2cAddress, void *data, uint16_t maxdatalen, uint32_t timeoutMs);
