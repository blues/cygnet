
#include "main.h"

#pragma once

// serial.c
void serialInit(uint32_t serialTaskID);
void serialPoll(void);
bool serialLock(UART_HandleTypeDef *huart, uint8_t **retData, uint32_t *retDataLen, bool *retDiagAllowed);
void serialUnlock(UART_HandleTypeDef *huart, bool reset);
void serialSetReqTaskID(uint32_t taskID);
void serialOutput(UART_HandleTypeDef *huart, uint8_t *buf, uint32_t buflen);
void serialOutputLn(UART_HandleTypeDef *huart, uint8_t *buf, uint32_t buflen);
