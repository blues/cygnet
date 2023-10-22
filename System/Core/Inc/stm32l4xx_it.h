// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#pragma once

void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void DebugMon_Handler(void);
void TIM2_IRQHandler(void);
void LPTIM1_IRQHandler(void);
void USB_IRQHandler(void);
void CAN1_TX_IRQHandler(void);
void CAN1_RX0_IRQHandler(void);
void CAN1_RX1_IRQHandler(void);
void CAN1_SCE_IRQHandler(void);
void I2C1_EV_IRQHandler(void);
void I2C1_ER_IRQHandler(void);
void I2C3_EV_IRQHandler(void);
void I2C3_ER_IRQHandler(void);
void LPUART1_IRQHandler(void);
void USART1_IRQHandler(void);
void USART2_IRQHandler(void);
void DMA1_Channel4_IRQHandler(void);
void DMA1_Channel5_IRQHandler(void);
void DMA1_Channel6_IRQHandler(void);
void DMA1_Channel7_IRQHandler(void);
void DMA2_Channel6_IRQHandler(void);
void DMA2_Channel7_IRQHandler(void);
void SPI1_IRQHandler(void);
void SPI2_IRQHandler(void);

