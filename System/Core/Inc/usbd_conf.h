// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "stm32l4xx.h"
#include "stm32l4xx_hal.h"

#define USBD_MAX_NUM_INTERFACES     1U
#define USBD_MAX_NUM_CONFIGURATION  1U
#define USBD_MAX_STR_DESC_SIZ       512U
#define USBD_DEBUG_LEVEL            0U
#define USBD_LPM_ENABLED            1U
#define USBD_SELF_POWERED           1U

// #define for FS and HS identification
#define DEVICE_FS 		0

#define USBD_malloc         (void *)USBD_static_malloc
#define USBD_free           USBD_static_free
#define USBD_memset         memset
#define USBD_memcpy         memcpy
#define USBD_Delay          HAL_Delay

// DEBUG macros

#if (USBD_DEBUG_LEVEL > 0)
#define USBD_UsrLog(...)    debugf(__VA_ARGS__);\
                            debugf("\n");
#else
#define USBD_UsrLog(...)
#endif

#if (USBD_DEBUG_LEVEL > 1)

#define USBD_ErrLog(...)    debugf("ERROR: ") ;\
                            debugf(__VA_ARGS__);\
                            debugf("\n");
#else
#define USBD_ErrLog(...)
#endif

#if (USBD_DEBUG_LEVEL > 2)
#define USBD_DbgLog(...)    debugf("DEBUG : ") ;\
                            debugf(__VA_ARGS__);\
                            debugf("\n");
#else
#define USBD_DbgLog(...)
#endif

void *USBD_static_malloc(uint32_t size);
void USBD_static_free(void *p);
