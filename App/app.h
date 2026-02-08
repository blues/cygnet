// Copyright 2024 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "main.h"
#include "mutex.h"
#include "product.h"

#pragma once

// App supports STOP2 mode
#define APP_SUPPORTS_STOP2_MODE     false
#define ENABLE_USART1               true
#define ENABLE_USART2               false

// Task parameters
#define TASKID_MAIN                 0           // Serial uart poller
#define TASKNAME_MAIN               "uart"
#define TASKLETTER_MAIN             'U'
#define TASKSTACK_MAIN              2000
#define TASKPRI_MAIN                ( configMAX_PRIORITIES - 1 )        // highest

#define TASKID_REQ                  1           // Request handler
#define TASKNAME_REQ                "request"
#define TASKLETTER_REQ              'R'
#define TASKSTACK_REQ               2500
#define TASKPRI_REQ                 ( configMAX_PRIORITIES - 2 )        // Normal

#define TASKID_NUM_TASKS            2           // Total
#define TASKID_UNKNOWN              0xFFFF
#define STACKWORDS(x)               ((x) / sizeof(StackType_t))

// serial.c
bool serialIsActive(void);
void serialInit(uint32_t serialTaskID);
void serialPoll(void);
bool serialIsDebugPort(UART_HandleTypeDef *huart);
bool serialSetDebugPort(UART_HandleTypeDef *huart);
bool serialLock(UART_HandleTypeDef *huart, uint8_t **retData, uint32_t *retDataLen, bool *retDiagAllowed);
void serialUnlock(UART_HandleTypeDef *huart, bool reset);
void serialOutputString(UART_HandleTypeDef *huart, char *buf);
void serialOutput(UART_HandleTypeDef *huart, uint8_t *buf, uint32_t buflen);
void serialOutputLn(UART_HandleTypeDef *huart, uint8_t *buf, uint32_t buflen);

// maintask.c
void mainTask(void *params);

// button.c
void buttonPressISR(bool pressed);

// led.c
void ledRestartSignal();
void ledEnable(bool enable);
bool ledIsEnabled(void);

// reqtask.c
#define rdtNone             0
#define rdtRestart          1
#define rdtBootloader       2
void reqTask(void *params);
void reqButtonPressedISR(void);

// req.c
err_t reqProcess(bool debugPort, uint8_t *reqJSON, bool diagAllowed);

// diag.c
err_t diagProcess(char *diagCommand);

// Errors
#define ERR_IO "{io}"
