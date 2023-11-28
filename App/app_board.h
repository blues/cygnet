// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#pragma once

#define MODEM_USART2
#define DEBUG_USART1
#define GPS_USART1
#define REQ_LPUART1

#define	MODEM_PWRKEY_NOD_Pin            D10_Pin
#define	MODEM_PWRKEY_NOD_GPIO_Port		D10_GPIO_Port

#define	MODEM_RESET_NOD_Pin             D9_Pin
#define	MODEM_RESET_NOD_GPIO_Port		D9_GPIO_Port

#define	MODEM_RI_Pin                    D12_Pin
#define	MODEM_RI_GPIO_Port              D12_GPIO_Port
#define MODEM_RI_EXTI_IRQn              D12_IRQn        // LINE15

#define	MODEM_PSM_EINT_NOD_Pin          D11_Pin
#define	MODEM_PSM_EINT_NOD_GPIO_Port    D11_GPIO_Port

#define	EN_MODEM_DFU_Pin				A4_Pin
#define	EN_MODEM_DFU_GPIO_Port			A4_GPIO_Port

#define	MAIN_POWER_Pin					D5_Pin
#define	MAIN_POWER_GPIO_Port			D5_GPIO_Port

#define	MODEM_POWER_NOD_Pin             D6_Pin
#define	MODEM_POWER_NOD_GPIO_Port       D6_GPIO_Port

#define	LED_BUSY_Pin                    LED_BUILTIN_Pin
#define	LED_BUSY_GPIO_Port              LED_BUILTIN_GPIO_Port

// We keep the test cert in the high page of flash
#define TESTCERT_FLASH_ADDRESS          ((void *)((FLASH_BASE+FLASH_SIZE)-FLASH_PAGE_SIZE))
#define TESTCERT_FLASH_LEN              FLASH_PAGE_SIZE

