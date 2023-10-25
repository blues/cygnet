// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#pragma once

#define MODEM_USART2
#define DEBUG_USART1
#define REQ_LPUART1

#define	MODEM_RESET_Pin                 D9_Pin
#define	MODEM_RESET_GPIO_Port			D9_GPIO_Port

#define	MODEM_PWRKEY_Pin                D10_Pin
#define	MODEM_PWRKEY_GPIO_Port			D10_GPIO_Port

#define	MODEM_RI_Pin                    D12_Pin
#define	MODEM_RI_GPIO_Port              D12_GPIO_Port
#define MODEM_RI_EXTI_IRQn              D12_IRQn

#define	MODEM_PSM_WAKE_Pin              D13_Pin
#define	MODEM_PSM_WAKE_GPIO_Port        D13_GPIO_Port

#define	MODEM_POWER_Pin                 D5_Pin
#define	MODEM_POWER_GPIO_Port			D5_GPIO_Port

#define	MODEM_DISCHARGE_Pin             D6_Pin
#define	MODEM_DISCHARGE_GPIO_Port       D6_GPIO_Port

#define	LED_BUSY_Pin                    LED_BUILTIN_Pin
#define	LED_BUSY_GPIO_Port              LED_BUILTIN_GPIO_Port

