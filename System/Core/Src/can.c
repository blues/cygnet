// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "can.h"

CAN_HandleTypeDef hcan1;

// CAN1 init function
void MX_CAN1_Init(void)
{

    hcan1.Instance = CAN1;
    hcan1.Init.Prescaler = 16;
    hcan1.Init.Mode = CAN_MODE_NORMAL;
    hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
    hcan1.Init.TimeSeg1 = CAN_BS1_3TQ;
    hcan1.Init.TimeSeg2 = CAN_BS2_1TQ;
    hcan1.Init.TimeTriggeredMode = DISABLE;
    hcan1.Init.AutoBusOff = DISABLE;
    hcan1.Init.AutoWakeUp = DISABLE;
    hcan1.Init.AutoRetransmission = DISABLE;
    hcan1.Init.ReceiveFifoLocked = DISABLE;
    hcan1.Init.TransmitFifoPriority = DISABLE;
    if (HAL_CAN_Init(&hcan1) != HAL_OK) {
        Error_Handler();
    }

}

// CAN msp init
void HAL_CAN_MspInit(CAN_HandleTypeDef* canHandle)
{

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if(canHandle->Instance==CAN1) {

        // CAN1 clock enable
        __HAL_RCC_CAN1_CLK_ENABLE();

        //CAN1 GPIO Configuration

        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = CAN1_AF;
        GPIO_InitStruct.Pin = CAN1_D6_TX_Pin;
        HAL_GPIO_Init(CAN1_D6_TX_GPIO_Port, &GPIO_InitStruct);
        GPIO_InitStruct.Pin = CAN1_D5_RX_Pin;
        HAL_GPIO_Init(CAN1_D5_RX_GPIO_Port, &GPIO_InitStruct);

		memset(&GPIO_InitStruct, 0, sizeof(GPIO_InitStruct));
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pin = CAN1_D12_STBY_Pin;
        HAL_GPIO_Init(CAN1_D12_STBY_GPIO_Port, &GPIO_InitStruct);
        HAL_GPIO_WritePin(CAN1_D12_STBY_GPIO_Port, CAN1_D12_STBY_Pin, GPIO_PIN_RESET);

        // CAN1 interrupt Init
        HAL_NVIC_SetPriority(CAN1_TX_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(CAN1_TX_IRQn);
        HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
        HAL_NVIC_SetPriority(CAN1_RX1_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(CAN1_RX1_IRQn);
        HAL_NVIC_SetPriority(CAN1_SCE_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(CAN1_SCE_IRQn);

    }
}

// CAN msp deinit
void HAL_CAN_MspDeInit(CAN_HandleTypeDef* canHandle)
{

    if(canHandle->Instance==CAN1) {

        // Peripheral clock disable
        __HAL_RCC_CAN1_CLK_DISABLE();

		// Deinit
        HAL_GPIO_DeInit(CAN1_D5_RX_GPIO_Port, CAN1_D5_RX_Pin);
        HAL_GPIO_DeInit(CAN1_D6_TX_GPIO_Port, CAN1_D6_TX_Pin);
        HAL_GPIO_DeInit(CAN1_D12_STBY_GPIO_Port, CAN1_D12_STBY_Pin);

        // CAN1 interrupt Deinit
        HAL_NVIC_DisableIRQ(CAN1_TX_IRQn);
        HAL_NVIC_DisableIRQ(CAN1_RX0_IRQn);
        HAL_NVIC_DisableIRQ(CAN1_RX1_IRQn);
        HAL_NVIC_DisableIRQ(CAN1_SCE_IRQn);

    }
}
