
#include "main.h"
#include "gmutex.h"
#include "json.h"

#pragma once

// Task parameters
#define TASKID_MAIN                 0           // Serial uart poller
#define TASKNAME_MAIN               "uart"
#define TASKLETTER_MAIN             'U'
#define TASKSTACK_MAIN              2000
#define TASKPRI_MAIN                ( configMAX_PRIORITIES - 1 )        // highest

#define TASKID_REQ                  1           // Request handler
#define TASKNAME_REQ                "request"
#define TASKLETTER_REQ              'R'
#define TASKSTACK_REQ               3500
#define TASKPRI_REQ                 ( configMAX_PRIORITIES - 2 )        // Normal

#define TASKID_NUM_TASKS            2   // Total
#define TASKID_UNKNOWN              0xFFFF
#define STACKWORDS(x)               ((x) / sizeof(StackType_t))

// serial.c
void serialInit(uint32_t serialTaskID);
void serialPoll(void);
bool serialLock(UART_HandleTypeDef *huart, uint8_t **retData, uint32_t *retDataLen, bool *retDiagAllowed);
void serialUnlock(UART_HandleTypeDef *huart, bool reset);
void serialSetReqTaskID(uint32_t taskID);
void serialOutputString(UART_HandleTypeDef *huart, char *buf);
void serialOutput(UART_HandleTypeDef *huart, uint8_t *buf, uint32_t buflen);
void serialOutputLn(UART_HandleTypeDef *huart, uint8_t *buf, uint32_t buflen);

// maintask.c
void mainTask(void *params);

// button.c
void buttonPressISR(bool pressed);

// led.c
void ledRestartSignal();
void ledBusy(bool enable);

// reqtask.c
#define rdtNone             0
#define rdtRestart          1
#define rdtBootloader       2
void reqTask(void *params);
void reqButtonPressedISR(void);

// req.c
err_t reqProcess(bool debugPort, uint8_t *reqJSON, uint32_t reqJSONLen, bool diagAllowed, uint8_t **rspJSON, uint32_t *rspJSONLen);

// diag.c
err_t diagProcess(char *diagCommand);

// Errors
#define ERR_IO "{io}"