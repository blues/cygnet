/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define USB_DETECT_Pin GPIO_PIN_13
#define USB_DETECT_GPIO_Port GPIOC
#define ENABLE_3V3_Pin GPIO_PIN_0
#define ENABLE_3V3_GPIO_Port GPIOH
#define DISCHARGE_3V3_Pin GPIO_PIN_1
#define DISCHARGE_3V3_GPIO_Port GPIOH
#define A0_Pin GPIO_PIN_0
#define A0_GPIO_Port GPIOA
#define A1_Pin GPIO_PIN_1
#define A1_GPIO_Port GPIOA
#define A2_Pin GPIO_PIN_2
#define A2_GPIO_Port GPIOA
#define A3_Pin GPIO_PIN_3
#define A3_GPIO_Port GPIOA
#define A4_Pin GPIO_PIN_4
#define A4_GPIO_Port GPIOA
#define CK_Pin GPIO_PIN_5
#define CK_GPIO_Port GPIOA
#define MI_Pin GPIO_PIN_6
#define MI_GPIO_Port GPIOA
#define A5_Pin GPIO_PIN_7
#define A5_GPIO_Port GPIOA
#define BAT_VOLTAGE_Pin GPIO_PIN_1
#define BAT_VOLTAGE_GPIO_Port GPIOB
#define LPUART1_RX_Pin GPIO_PIN_10
#define LPUART1_RX_GPIO_Port GPIOB
#define LPUART1_TX_Pin GPIO_PIN_11
#define LPUART1_TX_GPIO_Port GPIOB
#define D10_Pin GPIO_PIN_13
#define D10_GPIO_Port GPIOB
#define D9_Pin GPIO_PIN_14
#define D9_GPIO_Port GPIOB
#define D12_Pin GPIO_PIN_15
#define D12_GPIO_Port GPIOB
#define LED_BUILTIN_Pin GPIO_PIN_8
#define LED_BUILTIN_GPIO_Port GPIOA
#define TX_Pin GPIO_PIN_9
#define TX_GPIO_Port GPIOA
#define RX_Pin GPIO_PIN_10
#define RX_GPIO_Port GPIOA
#define CHARGE_DETECT_Pin GPIO_PIN_15
#define CHARGE_DETECT_GPIO_Port GPIOA
#define USER_BTN_Pin GPIO_PIN_3
#define USER_BTN_GPIO_Port GPIOB
#define D13_Pin GPIO_PIN_4
#define D13_GPIO_Port GPIOB
#define MO_Pin GPIO_PIN_5
#define MO_GPIO_Port GPIOB
#define SCL_Pin GPIO_PIN_6
#define SCL_GPIO_Port GPIOB
#define SDA_Pin GPIO_PIN_7
#define SDA_GPIO_Port GPIOB
#define B_Pin GPIO_PIN_3
#define B_GPIO_Port GPIOH
#define D5_Pin GPIO_PIN_8
#define D5_GPIO_Port GPIOB
#define D6_Pin GPIO_PIN_9
#define D6_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
