// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "i2c.h"

I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c3;

// I2C1 init function
void MX_I2C1_Init(void)
{

    hi2c1.Instance = I2C1;
    hi2c1.Init.Timing = 0x10909CEC;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
        Error_Handler();
    }

    // Configure Analogue filter
    if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK) {
        Error_Handler();
    }

    // Configure Digital filter
    if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK) {
        Error_Handler();
    }

}

// I2C3 init function
void MX_I2C3_Init(void)
{

    hi2c3.Instance = I2C3;
    hi2c3.Init.Timing = 0x10909CEC;
    hi2c3.Init.OwnAddress1 = 0;
    hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c3.Init.OwnAddress2 = 0;
    hi2c3.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c3) != HAL_OK) {
        Error_Handler();
    }

    // Configure Analogue filter
    if (HAL_I2CEx_ConfigAnalogFilter(&hi2c3, I2C_ANALOGFILTER_ENABLE) != HAL_OK) {
        Error_Handler();
    }

    // Configure Digital filter
    if (HAL_I2CEx_ConfigDigitalFilter(&hi2c3, 0) != HAL_OK) {
        Error_Handler();
    }

}

// I2C msp init
void HAL_I2C_MspInit(I2C_HandleTypeDef* i2cHandle)
{

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    if(i2cHandle->Instance==I2C1) {

        // Initializes the peripherals clock
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_I2C1;
        PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
            Error_Handler();
        }

        // I2C1 clock enable
        __HAL_RCC_I2C1_CLK_ENABLE();

        // I2C1 GPIO Configuration
        GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = I2C1_AF;
        GPIO_InitStruct.Pin = GPIO_SCL_Pin;
        HAL_GPIO_Init(GPIO_SCL_GPIO_Port, &GPIO_InitStruct);
        GPIO_InitStruct.Pin = GPIO_SDA_Pin;
        HAL_GPIO_Init(GPIO_SDA_GPIO_Port, &GPIO_InitStruct);

        // I2C1 interrupt Init
        HAL_NVIC_SetPriority(I2C1_EV_IRQn, INTERRUPT_PRIO_I2CM, 0);
        HAL_NVIC_EnableIRQ(I2C1_EV_IRQn);
        HAL_NVIC_SetPriority(I2C1_ER_IRQn, INTERRUPT_PRIO_I2CM, 0);
        HAL_NVIC_EnableIRQ(I2C1_ER_IRQn);

    }

    if(i2cHandle->Instance==I2C3) {

        // Initializes the peripherals clock
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_I2C3;
        PeriphClkInit.I2c3ClockSelection = RCC_I2C3CLKSOURCE_PCLK1;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
            Error_Handler();
        }

        // I2C3 clock enable
        __HAL_RCC_I2C3_CLK_ENABLE();

        // I2C3 GPIO Configuration
        GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = I2C3_AF;
        GPIO_InitStruct.Pin = I2C3_A5_SCL_Pin;
        HAL_GPIO_Init(I2C3_A5_SCL_GPIO_Port, &GPIO_InitStruct);
        GPIO_InitStruct.Pin = I2C3_D13_SDA_Pin;
        HAL_GPIO_Init(I2C3_D13_SDA_GPIO_Port, &GPIO_InitStruct);

        // I2C3 interrupt Init
        HAL_NVIC_SetPriority(I2C3_EV_IRQn, INTERRUPT_PRIO_I2CM, 0);
        HAL_NVIC_EnableIRQ(I2C3_EV_IRQn);
        HAL_NVIC_SetPriority(I2C3_ER_IRQn, INTERRUPT_PRIO_I2CM, 0);
        HAL_NVIC_EnableIRQ(I2C3_ER_IRQn);

    }

}

// I2C msp deinit
void HAL_I2C_MspDeInit(I2C_HandleTypeDef* i2cHandle)
{

    if(i2cHandle->Instance==I2C1) {

        // Peripheral clock disable
        __HAL_RCC_I2C1_CLK_DISABLE();

        // GPIO DeInit
        HAL_GPIO_DeInit(I2C1_SCL_GPIO_Port, I2C1_SCL_Pin);
        HAL_GPIO_DeInit(I2C1_SDA_GPIO_Port, I2C1_SDA_Pin);

        // I2C1 interrupt Deinit
        HAL_NVIC_DisableIRQ(I2C1_EV_IRQn);
        HAL_NVIC_DisableIRQ(I2C1_ER_IRQn);

    }

    if(i2cHandle->Instance==I2C3) {

        // Peripheral clock disable
        __HAL_RCC_I2C3_CLK_DISABLE();

        // GPIO DeInit
        HAL_GPIO_DeInit(I2C3_A5_SCL_GPIO_Port, I2C3_A5_SCL_Pin);
        HAL_GPIO_DeInit(I2C3_D13_SDA_GPIO_Port, I2C3_D13_SDA_Pin);

        // I2C3 interrupt Deinit
        HAL_NVIC_DisableIRQ(I2C3_EV_IRQn);
        HAL_NVIC_DisableIRQ(I2C3_ER_IRQn);

    }

}
