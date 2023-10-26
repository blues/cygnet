// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#pragma once

#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_exti.h"
#include "FreeRTOSConfig.h"

#define BOARD_NUCLEO		0       // Proto using NUCLEO-L433RC-P
#define BOARD_V1            1       // First spin
#define CURRENT_BOARD       BOARD_NUCLEO

// Pin Definitions for stm32l433cct6t
// 1 VBAT				13 PA3/IN8			25 PB12				37 PA14
// 2 PC13				14 PA4/IN9			26 PB13				38 PA15
// 3 PC14-OSC32_IN		15 PA5/IN10			27 PB14				39 PB3
// 4 PC15-OSC32_OUT		16 PA6/IN11			28 PB15				40 PB4
// 5 PH0-OSC_IN			17 PA7/IN12			29 PA8/TIM1CH1		41 PB5
// 6 PH1-OSC_OUT		18 PB0/IN15			30 PA9				42 PB6
// 7 NRST				19 PB1/IN16			31 PA10				43 PB7
// 8 VSSA/VREF-			20 PB2				32 PA11				44 PH3-BOOT0
// 9 VDDA/VREF+			21 PB10				33 PA12				45 PB8
// 10 PA0/IN5			22 PB11				34 PA13				46 PB9
// 11 PA1/IN6			23 VSS				35 VSS				47 VSS
// 12 PA2/IN7			24 VDD				36 VDDUSB			48 VDD

// NOTE these incompatibilities:
//   SPI2 or CAN1 because of D6/D12
//   USART2 or MOD1 or non-VCP LPUART1 because of A2/A3
//   VCP LPUART1 and non-VCP LPUART1

#if (CURRENT_BOARD != BOARD_NUCLEO)
#define	ENABLE_3V3_Pin					GPIO_PIN_0		// PH0
#define	ENABLE_3V3_GPIO_Port			GPIOH
#define	DISCHARGE_3V3_Pin				GPIO_PIN_1		// PH1
#define	DISCHARGE_3V3_GPIO_Port			GPIOH
#define	BAT_VOLTAGE_Pin					GPIO_PIN_4		// PA4
#define	BAT_VOLTAGE_GPIO_Port			GPIOA
#define	ADC_CHANNEL_BAT_VOLTAGE			ADC_CHANNEL_9
#define	CHARGE_DETECT_Pin				GPIO_PIN_15		// PA15
#define	CHARGE_DETECT_GPIO_Port			GPIOA
#endif

#if (CURRENT_BOARD != BOARD_NUCLEO)
#define	LED_BUILTIN_Pin					GPIO_PIN_8		// PA8
#define	LED_BUILTIN_GPIO_Port			GPIOA
#define	USER_BTN_Pin					GPIO_PIN_3		// PB3
#define	USER_BTN_GPIO_Port				GPIOB
#define	USER_BTN_STATE_PUSHED			GPIO_PIN_RESET
#define	USER_BTN_STATE_RELEASED			GPIO_PIN_SET
#define	USER_BTN_IRQn                   EXTI3_IRQn
#else
#define	LED_BUILTIN_Pin					GPIO_PIN_13		// PD13 (LD4)
#define	LED_BUILTIN_GPIO_Port			GPIOD
#define	USER_BTN_Pin					GPIO_PIN_13		// PB13 (B1 USER)
#define	USER_BTN_GPIO_Port				GPIOB
#define	USER_BTN_IRQn                   EXTI15_10_IRQn
#define	USER_BTN_STATE_PUSHED			GPIO_PIN_SET
#define	USER_BTN_STATE_RELEASED			GPIO_PIN_RESET
#endif

#if (CURRENT_BOARD != BOARD_NUCLEO)
#define	USB_DETECT_Pin					GPIO_PIN_13		// PC13
#define	USB_DETECT_GPIO_Port			GPIOC
#define	USB_DETECT_IRQn                 EXTI15_10_IRQn
#define	USB_DM_Pin						GPIO_PIN_11		// PA11
#define	USB_DM_GPIO_Port				GPIOA
#define	USB_DP_Pin						GPIO_PIN_12		// PA12
#define	USB_DP_GPIO_Port				GPIOA
#define	USB_AF							GPIO_AF10_USB_FS
#endif

#define	SWDIO_Pin						GPIO_PIN_13		// PA13
#define	SWDIO_GPIO_Port					GPIOA
#define	SWCLK_Pin						GPIO_PIN_14		// PA14
#define	SWCLK_GPIO_Port					GPIOA

#define	BOOT_Pin						GPIO_PIN_3		// PH3
#define	BOOT_GPIO_Port					GPIOH

#define	A0_Pin							GPIO_PIN_0		// PA0
#define	A0_GPIO_Port					GPIOA
#define	A0_IRQn							EXTI0_IRQn
#define	ADC_CHANNEL_A0					ADC_CHANNEL_5
#define	A1_Pin							GPIO_PIN_1		// PA1
#define	A1_GPIO_Port					GPIOA
#define	A1_IRQn							EXTI1_IRQn
#define	ADC_CHANNEL_A1					ADC_CHANNEL_6
#define	A2_Pin							GPIO_PIN_2		// PA2
#define	A2_GPIO_Port					GPIOA
#define	A2_IRQn							EXTI2_IRQn
#define	ADC_CHANNEL_A2					ADC_CHANNEL_7
#define	A3_Pin							GPIO_PIN_3		// PA3
#define	A3_GPIO_Port					GPIOA
#define	A3_IRQn							EXTI3_IRQn
#define	ADC_CHANNEL_A3					ADC_CHANNEL_8
#define	A4_Pin							GPIO_PIN_1		// PB1
#define	A4_GPIO_Port					GPIOB
#define	A4_IRQn							EXTI1_IRQn
#define	ADC_CHANNEL_A4					ADC_CHANNEL_16
#define	A5_Pin							GPIO_PIN_7		// PA7
#define	A5_GPIO_Port					GPIOA
#define	A5_IRQn							EXTI9_5_IRQn
#define	ADC_CHANNEL_A5					ADC_CHANNEL_12

#define	D5_Pin							GPIO_PIN_8		// PB8
#define	D5_GPIO_Port					GPIOB
#define	D5_IRQn							EXTI9_5_IRQn
#define	D6_Pin							GPIO_PIN_9		// PB9
#define	D6_GPIO_Port					GPIOB
#define	D6_IRQn							EXTI9_5_IRQn
#define	D9_Pin							GPIO_PIN_14		// PB14
#define	D9_GPIO_Port					GPIOB
#define	D9_IRQn							EXTI15_10_IRQn
#define	D10_Pin							GPIO_PIN_13		// PB13
#define	D10_GPIO_Port					GPIOB
#define	D10_IRQn						EXTI15_10_IRQn
#define	D11_Pin							GPIO_PIN_0		// PB0
#define	D11_GPIO_Port					GPIOB
#define	D11_IRQn						EXTI0_IRQn
#define	ADC_CHANNEL_D11					ADC_CHANNEL_15
#define	D12_Pin							GPIO_PIN_15		// PB15
#define	D12_GPIO_Port					GPIOB
#define	D12_IRQn						EXTI15_10_IRQn
#define	D13_Pin							GPIO_PIN_4		// PB4
#define	D13_GPIO_Port					GPIOB
#define	D13_IRQn						EXTI4_IRQn

#define	CK_Pin							GPIO_PIN_5		// PA5
#define	CK_GPIO_Port					GPIOA
#define	ADC_CHANNEL_CK					ADC_CHANNEL_10
#define	MI_Pin							GPIO_PIN_6		// PA6
#define	MI_GPIO_Port					GPIOA
#define	ADC_CHANNEL_MI					ADC_CHANNEL_11
#define	MO_Pin							GPIO_PIN_5		// PB5
#define	MO_GPIO_Port					GPIOB
#define	CS_Pin							D11_Pin
#define	CS_GPIO_Port					D11_GPIO_Port

#define	SPI1_CK_Pin						CK_Pin
#define	SPI1_CK_GPIO_Port				CK_GPIO_Port
#define	SPI1_MI_Pin						MI_Pin
#define	SPI1_MI_GPIO_Port				MI_GPIO_Port
#define	SPI1_MO_Pin						MO_Pin
#define	SPI1_MO_GPIO_Port				MO_GPIO_Port
#define	SPI1_CS_Pin						CS_Pin
#define	SPI1_CS_GPIO_Port				CS_GPIO_Port
#define	SPI1_AF							GPIO_AF5_SPI1

#define	SPI2_D10_CK_Pin					D10_Pin
#define	SPI2_D10_CK_GPIO_Port			D10_GPIO_Port
#define	SPI2_D9_MI_Pin					D9_Pin
#define	SPI2_D9_MI_GPIO_Port			D9_GPIO_Port
#define	SPI2_D12_MO_Pin					D12_Pin
#define	SPI2_D12_MO_GPIO_Port			D12_GPIO_Port
#define	SPI2_D6_CS_Pin					D6_Pin
#define	SPI2_D6_CS_GPIO_Port			D6_GPIO_Port
#define	SPI2_AF							GPIO_AF5_SPI2
#define SPI2_RX_DMA_Channel				DMA1_Channel4
#define SPI2_RX_DMA_IRQn				DMA1_Channel4_IRQn
#define SPI2_RX_DMA_IRQHandler			DMA1_Channel4_IRQHandler
#define SPI2_TX_DMA_Channel				DMA1_Channel5
#define SPI2_TX_DMA_IRQn				DMA1_Channel5_IRQn
#define SPI2_TX_DMA_IRQHandler			DMA1_Channel5_IRQHandler

#define	LPUART1_VCP_RX_Pin				GPIO_PIN_10		// PB10
#define	LPUART1_VCP_RX_GPIO_Port		GPIOB
#define	LPUART1_VCP_TX_Pin				GPIO_PIN_11		// PB11
#define	LPUART1_VCP_TX_GPIO_Port		GPIOB
#define	LPUART1_A3_RX_Pin				A3_Pin
#define	LPUART1_A3_RX_GPIO_Port			A3_GPIO_Port
#define	LPUART1_A2_TX_Pin				A2_Pin
#define	LPUART1_A2_TX_GPIO_Port			A2_GPIO_Port
#define	LPUART1_AF						GPIO_AF8_LPUART1
#define	LPUART1_BAUDRATE				9600

#define	TX_Pin							GPIO_PIN_9		// PA9
#define	TX_GPIO_Port					GPIOA
#define	RX_Pin							GPIO_PIN_10		// PA10
#define	RX_GPIO_Port					GPIOA
#define	USART1_TX_Pin					TX_Pin
#define	USART1_TX_GPIO_Port				TX_GPIO_Port
#define	USART1_RX_Pin					RX_Pin
#define	USART1_RX_GPIO_Port				RX_GPIO_Port
#define	USART1_AF						GPIO_AF7_USART1
#define	USART1_BAUDRATE					115200
#define USART1_RX_DMA_Channel			DMA2_Channel7
#define USART1_RX_DMA_IRQn				DMA2_Channel7_IRQn
#define USART1_RX_DMA_IRQHandler		DMA2_Channel7_IRQHandler
#define USART1_TX_DMA_Channel			DMA2_Channel6
#define USART1_TX_DMA_IRQn				DMA2_Channel6_IRQn
#define USART1_TX_DMA_IRQHandler		DMA2_Channel6_IRQHandler

#define	USART2_A2_TX_Pin				A2_Pin
#define	USART2_A2_TX_GPIO_Port			A2_GPIO_Port
#define	USART2_A3_RX_Pin				A3_Pin
#define	USART2_A3_RX_GPIO_Port			A3_GPIO_Port
#define	USART2_AF						GPIO_AF7_USART2
#define	USART2_BAUDRATE					115200
#define USART2_RX_DMA_Channel			DMA1_Channel6
#define USART2_RX_DMA_IRQn				DMA1_Channel6_IRQn
#define USART2_RX_DMA_IRQHandler		DMA1_Channel6_IRQHandler
#define USART2_TX_DMA_Channel			DMA1_Channel7
#define USART2_TX_DMA_IRQn				DMA1_Channel7_IRQn
#define USART2_TX_DMA_IRQHandler		DMA1_Channel7_IRQHandler

#define	GPIO_SCL_Pin					GPIO_PIN_6		// PB6
#define	GPIO_SCL_GPIO_Port				GPIOB
#define	GPIO_SDA_Pin					GPIO_PIN_7		// PB7
#define	GPIO_SDA_GPIO_Port				GPIOB
#define	I2C1_SCL_Pin					GPIO_SCL_Pin
#define	I2C1_SCL_GPIO_Port				GPIO_SCL_GPIO_Port
#define	I2C1_SDA_Pin					GPIO_SDA_Pin
#define	I2C1_SDA_GPIO_Port				GPIO_SDA_GPIO_Port
#define	I2C1_AF							GPIO_AF4_I2C1

#define	I2C3_A5_SCL_Pin					A5_Pin
#define	I2C3_A5_SCL_GPIO_Port			A5_GPIO_Port
#define	I2C3_D13_SDA_Pin				D13_Pin
#define	I2C3_D13_SDA_GPIO_Port			D13_GPIO_Port
#define	I2C3_AF							GPIO_AF4_I2C3

#define	CAN1_D6_TX_Pin					D6_Pin
#define	CAN1_D6_TX_GPIO_Port			D6_GPIO_Port
#define	CAN1_D5_RX_Pin					D5_Pin
#define	CAN1_D5_RX_GPIO_Port			D5_GPIO_Port
#define	CAN1_D12_STBY_Pin				D12_Pin
#define	CAN1_D12_STBY_GPIO_Port			D12_GPIO_Port
#define	CAN1_AF							GPIO_AF9_CAN1

#define	RS485_A2_TX_Pin					A2_Pin
#define	RS485_A2_TX_GPIO_Port			A2_GPIO_Port
#define	RS485_A3_RX_Pin					A3_Pin
#define	RS485_A3_RX_GPIO_Port			A3_GPIO_Port
#define	RS485_A1_DE_Pin					A1_Pin
#define	RS485_A1_DE_GPIO_Port			A1_GPIO_Port
#define	RS485_AF						GPIO_AF7_USART2
#define	RS485_BAUDRATE					9600

// Interrupt priorities.  (Note - to get this you must include FreeRTOSCOnfig.h before board.h)
#ifdef configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY

// RTC is our highest priority, because it is our watchdog
#define INTERRUPT_PRIO_RTC				configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY
#define INTERRUPT_PRIO_TIMER			configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY
#define INTERRUPT_PRIO_TAMP				configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY

// Next are interrupts where external devices are reliant upon our timing/responsiveness
#define INTERRUPT_PRIO_I2CS				configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY+1
#define INTERRUPT_PRIO_I2CM				configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY+1
#define INTERRUPT_PRIO_SPI				configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY+1
#define	INTERRUPT_PRIO_USB				configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY+2
#define	INTERRUPT_PRIO_SERIAL			configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY+2
#define	INTERRUPT_PRIO_CAN				configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY+2
#define	INTERRUPT_PRIO_EXTI				configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY+3
#define INTERRUPT_PRIO_ADC				configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY+3

// This is used by "yield" in the task scheduler and should be lowest priority
#define INTERRUPT_PRIO_PENDSV			configLIBRARY_LOWEST_INTERRUPT_PRIORITY

#endif

// Temperature sensor
#define TEMPSENSOR_TYP_CAL1_V           (( int32_t)  760)   // Internal temperature sensor, parameter V30 (unit: mV). Refer to device datasheet for min/typ/max values.
#define TEMPSENSOR_TYP_AVGSLOPE         (( int32_t) 2500)   // Internal temperature sensor, parameter Avg_Slope (unit: uV/DegCelsius). Refer to device datasheet for min/typ/max vlaues.

// External voltage divider
#define VADC_DIV_TOP_RESISTOR           (float)10           // 10M Ohm
#define VADC_DIV_BOT_RESISTOR           (float)4.3          // 4M3 Ohm
#define VADC_DIV_K                      ((double)((VADC_DIV_TOP_RESISTOR + VADC_DIV_BOT_RESISTOR) / VADC_DIV_BOT_RESISTOR))

// Raw data acquired at a temperature of 30 deg C (+-5 deg C)
// VDDA = VREF+ = 3.0 V (+- 10 mV)
#define STM32_VDDA_REF_VOLT				(float)(3.00)

// RTC
#define RTC_N_PREDIV_S                  10
#define RTC_PREDIV_S                    ((1<<RTC_N_PREDIV_S)-1)
#define RTC_PREDIV_A                    ((1<<(15-RTC_N_PREDIV_S))-1)

// App's overrides to board.h, which are resolved in the "weak" folder if the app doesn't define them
#include "app_board.h"
