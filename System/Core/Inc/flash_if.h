// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#pragma once

#include "main.h"

typedef enum {
    FLASH_IF_PARAM_ERROR  = -6, /*!< Error Flash invalid parameter */
    FLASH_IF_LOCK_ERROR   = -5, /*!< Error Flash not locked */
    FLASH_IF_WRITE_ERROR  = -4, /*!< Error Flash write not possible */
    FLASH_IF_READ_ERROR   = -3, /*!< Error Flash read not possible */
    FLASH_IF_ERASE_ERROR  = -2, /*!< Error Flash erase not possible */
    FLASH_IF_ERROR        = -1, /*!< Error Flash generic */
    FLASH_IF_OK           = 0,  /*!< Flash Success */
    FLASH_IF_BUSY         = 1   /*!< Flash not available */
} FLASH_IF_StatusTypedef;

FLASH_IF_StatusTypedef FLASH_IF_Init(void *pAllocRamBuffer);
FLASH_IF_StatusTypedef FLASH_IF_DeInit(void);
FLASH_IF_StatusTypedef FLASH_IF_Write(void *pDestination, const void *pSource, uint32_t uLength);
FLASH_IF_StatusTypedef FLASH_IF_Read(void *pDestination, const void *pSource, uint32_t uLength);
FLASH_IF_StatusTypedef FLASH_IF_Erase(void *pStart, uint32_t uLength);
