// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "usart.h"

UART_HandleTypeDef hlpuart1;
bool lpuart1UsingAlternatePins = false;

UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart1_tx;

bool usart2UsingRS485 = false;
UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_rx;
DMA_HandleTypeDef hdma_usart2_tx;

// Callbacks
void (*TxCpltCallback_LPUART1)(void *) = NULL;
void (*TxCpltCallback_USART1)(void *) = NULL;
void (*TxCpltCallback_USART2)(void *) = NULL;

// Register a completion callback
void MX_UART_TxCpltCallback(UART_HandleTypeDef *huart, void (*cb)(void *))
{
    if (huart == &hlpuart1) {
        TxCpltCallback_LPUART1 = cb;
    }
    if (huart == &huart1) {
        TxCpltCallback_USART1 = cb;
    }
    if (huart == &huart2) {
        TxCpltCallback_USART2 = cb;
    }
}

// Transmit on LPUART1 (for low level debugging only)
void MX_LPUART1_Message(char *buf)
{
    MX_UART_Transmit(&hlpuart1, (uint8_t *)buf, strlen(buf), 500);
}

// Transmit to a port synchronously
void MX_UART_Transmit(UART_HandleTypeDef *huart, uint8_t *buf, uint32_t len, uint32_t timeoutMs)
{

    // Transmit
    bool waitWhileUartBusy = false;
    if (huart == &hlpuart1) {
        HAL_UART_Transmit_IT(huart, buf, len);
        waitWhileUartBusy = true;
    }
    if (huart == &huart1) {
        HAL_UART_Transmit_DMA(huart, buf, len);
        waitWhileUartBusy = true;
    }
    if (huart == &huart2) {
        HAL_UART_Transmit_DMA(huart, buf, len);
        waitWhileUartBusy = true;
    }

    // Wait, so that the caller won't mess with the buffer while the HAL is using it
    for (uint32_t i=0; waitWhileUartBusy && i<timeoutMs; i++) {
        HAL_UART_StateTypeDef state = HAL_UART_GetState(huart);
        if ((state & HAL_UART_STATE_BUSY_TX) != HAL_UART_STATE_BUSY_TX) {
            break;
        }
        HAL_Delay(1);
    }

}

// Transmit complete callback for serial ports
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == &hlpuart1 && TxCpltCallback_LPUART1 != NULL) {
        TxCpltCallback_LPUART1(huart);
    }
    if (huart == &huart1 && TxCpltCallback_USART1 != NULL) {
        TxCpltCallback_USART1(huart);
    }
    if (huart == &huart2 && TxCpltCallback_USART2 != NULL) {
        TxCpltCallback_USART2(huart);
    }
}

// Start a receive
void MX_UART_RxStart(UART_HandleTypeDef *huart)
{
    if (huart == &hlpuart1) {
        if (rxioLPUART1.buf == NULL) {
            return;
        }
        rxioLPUART1.rxlen = UART_RXLEN;
        HAL_UART_Receive_IT(huart, rxioLPUART1.iobuf, rxioUSART1.rxlen);
    }
    if (huart == &huart1) {
        if (rxioUSART1.buf == NULL) {
            return;
        }
        rxioUSART1.rxlen = UART_RXLEN;
        HAL_UART_Receive_DMA(huart, rxioUSART1.iobuf, rxioUSART1.rxlen);
    }
    if (huart == &huart2) {
        if (rxioUSART2.buf == NULL) {
            return;
        }
        rxioUSART2.rxlen = UART_RXLEN;
        HAL_UART_Receive_DMA(huart, rxioUSART2.iobuf, rxioUSART2.rxlen);
    }
}

// Register a completion callback and allocate receive buffer
void MX_UART_RxConfigure(UART_HandleTypeDef *huart, uint8_t *rxbuf, uint16_t rxbuflen, void (*cb)(UART_HandleTypeDef *huart, bool error))
{

    // Initialize receive buffers
    if (huart == &hlpuart1) {
        rxioLPUART1.buf = rxbuf;
        rxioLPUART1.buflen = rxbuflen;
        rxioLPUART1.fill = rxioLPUART1.drain = rxioLPUART1.rxlen = 0;
        rxioLPUART1.notifyReceivedFn = cb;
        err_t err = memAlloc(UART_RXLEN, &rxioLPUART1.iobuf);
        if (err) {
            debugPanic("lpuart1 iobuf");
        }
    }
    if (huart == &huart1) {
        rxioUSART1.buf = rxbuf;
        rxioUSART1.buflen = rxbuflen;
        rxioUSART1.fill = rxioUSART1.drain = rxioUSART1.rxlen =  0;
        rxioUSART1.notifyReceivedFn = cb;
        err_t err = memAlloc(UART_RXLEN, &rxioUSART1.iobuf);
        if (err) {
            debugPanic("usart1 iobuf");
        }
    }
    if (huart == &huart2) {
        rxioUSART2.buf = rxbuf;
        rxioUSART2.buflen = rxbuflen;
        rxioUSART2.fill = rxioUSART2.drain = rxioUSART2.rxlen =  0;
        rxioUSART2.notifyReceivedFn = cb;
        err_t err = memAlloc(UART_RXLEN, &rxioUSART2.iobuf);
        if (err) {
            debugPanic("usart2 iobuf");
        }
    }
}

// Add from the iobuf to the ring buffer, return true if success else false for failure
bool uioReceivedBytes(UARTIO *uio, uint8_t *buf, uint32_t buflen)
{
    for (int i=0; i<buflen; i++) {
        if (uio->fill >= uio->buflen) {
            debugPanic("ui");
        }
        // Always write the last byte, even if overrun.  This ensures
        // that a \n terminator will get into the buffer.
        uio->buf[uio->fill] = *buf++;
        if (uio->fill+1 == uio->drain) {
            // overrun - don't increment the pointer
            return false;
        }
        uio->fill++;
        if (uio->fill >= uio->buflen) {
            uio->fill = 0;
        }
    }
    return true;
}

// Receive complete
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    UARTIO *uio;
    uint16_t receivedBytes = 0;
    if (huart == &hlpuart1) {
        receivedBytes = rxioLPUART1.rxlen - huart->RxXferCount;
        uio = &rxioLPUART1;
    }
    if (huart == &huart1) {
        receivedBytes = rxioUSART1.rxlen - __HAL_DMA_GET_COUNTER(huart->hdmarx);
        uio = &rxioUSART1;
    }
    if (huart == &huart2) {
        receivedBytes = rxioUSART2.rxlen - __HAL_DMA_GET_COUNTER(huart->hdmarx);
        uio = &rxioUSART2;
    }
    if (huart->ErrorCode != HAL_UART_ERROR_NONE) {
        HAL_UART_Abort(huart);
        uio->fill = uio->drain = uio->rxlen = 0;
        if (uio->notifyReceivedFn != NULL) {
            uio->notifyReceivedFn(huart, true);
        }
    } else {
        if (!uioReceivedBytes(uio, uio->iobuf, receivedBytes)) {
            // Overrun error
            MX_Breakpoint();
            if (uio->notifyReceivedFn != NULL) {
                uio->notifyReceivedFn(huart, true);
            }
        } else {
            if (uio->notifyReceivedFn != NULL) {
                uio->notifyReceivedFn(huart, false);
            }
        }
    }
    MX_UART_RxStart(huart);
}

// See if anything is available
bool HAL_UART_RxAvailable(UART_HandleTypeDef *huart)
{
    UARTIO *uio;
    if (huart == &hlpuart1) {
        uio = &rxioLPUART1;
    }
    if (huart == &huart1) {
        uio = &rxioUSART1;
    }
    if (huart == &huart2) {
        uio = &rxioUSART2;
    }
    return (uio->fill != uio->drain);
}

// Get a byte from the receive buffer
// Note: only call this if RxAvailable returns true
uint8_t HAL_UART_RxGet(UART_HandleTypeDef *huart)
{
    UARTIO *uio;
    if (huart == &hlpuart1) {
        uio = &rxioLPUART1;
    }
    if (huart == &huart1) {
        uio = &rxioUSART1;
    }
    if (huart == &huart2) {
        uio = &rxioUSART2;
    }
    uint8_t databyte = uio->buf[uio->drain++];
    if (uio->drain >= uio->buflen) {
        uio->drain = 0;
    }
    return databyte;
}

// For UART receive
// UART receive I/O descriptor.  Note that if we ever fill the buffer
// completely we will get an I/O error because we can't start a receive
// without overwriting the data waiting to be pulled out.  There must
// always be at least one byte available to start a receive.
typedef struct {
    uint8_t *iobuf;
    uint8_t *buf;
    uint16_t buflen;
    uint16_t fill;
    uint16_t drain;
    uint16_t rxlen;
    void (*notifyReceivedFn)(UART_HandleTypeDef *huart, bool error);
} UARTIO;
STATIC UARTIO rxioLPUART1 = {0};
STATIC UARTIO rxioUSART1 = {0};
STATIC UARTIO rxioUSART2 = {0};

// Number of bytes for UART receives.  (We can just use this until there's a perf issue)
#define UART_RXLEN 1

// Forwards
bool uioReceivedBytes(UARTIO *uio, uint8_t *buf, uint32_t buflen);

// LPUART1 init function
void MX_LPUART1_UART_Init(bool altPins, uint32_t baudRate)
{
    lpuart1UsingAlternatePins = altPins;

    hlpuart1.Instance = LPUART1;
    hlpuart1.Init.BaudRate = baudRate;
    hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
    hlpuart1.Init.StopBits = UART_STOPBITS_1;
    hlpuart1.Init.Parity = UART_PARITY_NONE;
    hlpuart1.Init.Mode = UART_MODE_TX_RX;
    hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&hlpuart1) != HAL_OK) {
        Error_Handler();
    }

    if (HAL_UARTEx_SetTxFifoThreshold(&hlpuart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_UARTEx_SetRxFifoThreshold(&hlpuart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_UARTEx_DisableFifoMode(&hlpuart1) != HAL_OK) {
        Error_Handler();
    }

    MX_UART_RxStart(&hlpuart1);

    peripherals |= PERIPHERAL_LPUART1;

}

// LPUART1 suspend function
void MX_LPUART1_UART_Suspend(void)
{

    // Set wakeUp event on start bit
    UART_WakeUpTypeDef WakeUpSelection;
    WakeUpSelection.WakeUpEvent = UART_WAKEUP_ON_STARTBIT;
    HAL_UARTEx_StopModeWakeUpSourceConfig(&hlpuart1, WakeUpSelection);

    // Enable interrupt
    __HAL_UART_ENABLE_IT(&hlpuart1, UART_IT_WUF);

    // Clear OVERRUN flag
    LL_LPUART_ClearFlag_ORE(LPUART1);

    // Configure LPUART1 transfer interrupts by clearing the WUF
    // flag and enabling the UART Wake Up from stop mode interrupt
    LL_LPUART_ClearFlag_WKUP(LPUART1);
    LL_LPUART_EnableIT_WKUP(LPUART1);

    // Enable Wake Up From Stop
    LL_LPUART_EnableInStopMode(LPUART1);

}

// LPUART1 resume function
void MX_LPUART1_UART_Resume(void)
{
}

// Transmit to LPUART1
void MX_LPUART1_UART_Transmit(uint8_t *buf, uint32_t len, uint32_t timeoutMs)
{

    // Transmit
    HAL_UART_Transmit_IT(&hlpuart1, buf, len);

    // Wait, so that the caller won't mess with the buffer while the HAL is using it
    for (uint32_t i=0; i<timeoutMs; i++) {
        HAL_UART_StateTypeDef state = HAL_UART_GetState(&hlpuart1);
        if ((state & HAL_UART_STATE_BUSY_TX) != HAL_UART_STATE_BUSY_TX) {
            break;
        }
        HAL_Delay(1);
    }

}

// LPUART1 De-Initialization Function
void MX_LPUART1_UART_DeInit(void)
{
    peripherals &= ~PERIPHERAL_LPUART1;
    rxioLPUART1.fill = rxioLPUART1.drain = 0;
    __HAL_RCC_USART1_FORCE_RESET();
    __HAL_RCC_USART1_RELEASE_RESET();
    HAL_UART_DeInit(&hlpuart1);
}

// USART1 init function
void MX_USART1_UART_Init(uint32_t baudRate)
{

    huart1.Instance = USART1;
    huart1.Init.BaudRate = baudRate;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&huart1) != HAL_OK) {
        Error_Handler();
    }

    if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK) {
        Error_Handler();
    }

    MX_UART_RxStart(&huart1);

    peripherals |= PERIPHERAL_USART1;

}

// USART1 Deinitialization
void MX_USART1_UART_DeInit(void)
{

    // Deinitialized
    peripherals &= ~PERIPHERAL_USART1;

    // Deconfigure RX buffer
    rxioUSART1.fill = rxioUSART1.drain = 0;

    // Stop any pending DMA, if any
    HAL_UART_DMAStop(&huart1);

    // Deinit DMA interrupts
    HAL_NVIC_DisableIRQ(USART1_RX_DMA_IRQn);
    HAL_NVIC_DisableIRQ(USART1_TX_DMA_IRQn);

    // Reset peripheral
    __HAL_RCC_USART1_FORCE_RESET();
    __HAL_RCC_USART1_RELEASE_RESET();

    // Disable IDLE interrupt
    __HAL_UART_DISABLE_IT(&huart1, UART_IT_IDLE);

    // Deinit
    HAL_UART_DeInit(&huart1);

}

// USART2 init function
void MX_USART2_UART_Init(bool useRS485, uint32_t baudRate)
{
    usart2UsingRS485 = useRS485;

    huart2.Instance = USART2;
    huart2.Init.BaudRate = baudRate;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&huart2) != HAL_OK) {
        Error_Handler();
    }

    peripherals |= PERIPHERAL_USART2;

}

// USART2 Deinitialization
void MX_USART2_UART_DeInit(void)
{

    // Deinitialized
    peripherals &= ~PERIPHERAL_USART2;

    // Deconfigure RX buffer
    rxioUSART2.fill = rxioUSART2.drain = 0;

    // Stop any pending DMA, if any
    HAL_UART_DMAStop(&huart2);

    // Deinit DMA interrupts
    HAL_NVIC_DisableIRQ(USART1_RX_DMA_IRQn);
    HAL_NVIC_DisableIRQ(USART1_TX_DMA_IRQn);

    // Reset peripheral
    __HAL_RCC_USART1_FORCE_RESET();
    __HAL_RCC_USART1_RELEASE_RESET();

    // Disable IDLE interrupt
    __HAL_UART_DISABLE_IT(&huart2, UART_IT_IDLE);

    // Deinit
    HAL_UART_DeInit(&huart2);

}

// UART msp init
void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    if (uartHandle->Instance==LPUART1) {

        // Initializes the peripherals clock
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_LPUART1;
        PeriphClkInit.Lpuart1ClockSelection = RCC_LPUART1CLKSOURCE_PCLK1;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
            Error_Handler();
        }

        // LPUART1 clock enable
        __HAL_RCC_LPUART1_CLK_ENABLE();

        // GPIO Configuration
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = LPUART1_AF;
        if (!lpuart1UsingAlternatePins) {
            GPIO_InitStruct.Pin = LPUART1_VCP_RX_Pin;
            HAL_GPIO_Init(LPUART1_VCP_RX_GPIO_Port, &GPIO_InitStruct);
            GPIO_InitStruct.Pin = LPUART1_VCP_TX_Pin;
            HAL_GPIO_Init(LPUART1_VCP_TX_GPIO_Port, &GPIO_InitStruct);
        } else {
            GPIO_InitStruct.Pin = LPUART1_A3_RX_Pin;
            HAL_GPIO_Init(LPUART1_A3_RX_GPIO_Port, &GPIO_InitStruct);
            GPIO_InitStruct.Pin = LPUART1_A2_TX_Pin;
            HAL_GPIO_Init(LPUART1_A2_TX_GPIO_Port, &GPIO_InitStruct);
        }

        // LPUART1 interrupt Init
        HAL_NVIC_SetPriority(LPUART1_IRQn, INTERRUPT_PRIO_SERIAL, 0);
        HAL_NVIC_EnableIRQ(LPUART1_IRQn);

    }

    if (uartHandle->Instance==USART1) {

        // Initializes the peripherals clock
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
        PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
            Error_Handler();
        }

        // USART1 clock enable
        MX_DMA_Init();
        __HAL_RCC_USART1_CLK_ENABLE();

        // GPIO Configuration
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = USART1_AF;
        GPIO_InitStruct.Pin = USART1_TX_Pin
                              HAL_GPIO_Init(USART1_TX_GPIO_Port, &GPIO_InitStruct);
        GPIO_InitStruct.Pin = USART1_RX_Pin
                              HAL_GPIO_Init(USART1_RX_GPIO_Port, &GPIO_InitStruct);

        // USART1_RX Init
        hdma_usart1_rx.Instance = USART1_RX_DMA_Channel;
        hdma_usart1_rx.Init.Request = DMA_REQUEST_2;
        hdma_usart1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_usart1_rx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_usart1_rx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_usart1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_usart1_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_usart1_rx.Init.Mode = DMA_NORMAL;
        hdma_usart1_rx.Init.Priority = DMA_PRIORITY_LOW;
        if (HAL_DMA_Init(&hdma_usart1_rx) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(uartHandle,hdmarx,hdma_usart1_rx);

        // USART1_TX Init
        hdma_usart1_tx.Instance = USART1_TX_DMA_Channel;
        hdma_usart1_tx.Init.Request = DMA_REQUEST_2;
        hdma_usart1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_usart1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_usart1_tx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_usart1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_usart1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_usart1_tx.Init.Mode = DMA_NORMAL;
        hdma_usart1_tx.Init.Priority = DMA_PRIORITY_LOW;
        if (HAL_DMA_Init(&hdma_usart1_tx) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(uartHandle,hdmatx,hdma_usart1_tx);

        // USART1 interrupt Init
        HAL_NVIC_SetPriority(USART1_IRQn, INTERRUPT_PRIO_SERIAL, 0);
        HAL_NVIC_EnableIRQ(USART1_IRQn);
        HAL_NVIC_SetPriority(USART1_RX_DMA_IRQn, INTERRUPT_PRIO_SERIAL, 0);
        HAL_NVIC_EnableIRQ(USART1_RX_DMA_IRQn);
        HAL_NVIC_SetPriority(USART1_TX_DMA_IRQn, INTERRUPT_PRIO_SERIAL, 0);
        HAL_NVIC_EnableIRQ(USART1_TX_DMA_IRQn);

    }

    if (uartHandle->Instance==USART2) {

        // Initializes the peripherals clock
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2;
        PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
            Error_Handler();
        }

        // USART2 clock enable
        MX_DMA_Init();
        __HAL_RCC_USART2_CLK_ENABLE();

        // GPIO Configuration
        if (!usart2UsingRS485) {
            GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
            GPIO_InitStruct.Pull = GPIO_NOPULL;
            GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
            GPIO_InitStruct.Alternate = USART2_AF;
            GPIO_InitStruct.Pin = USART2_A2_TX_Pin;
            HAL_GPIO_Init(USART2_A2_TX_GPIO_Port, &GPIO_InitStruct);
            GPIO_InitStruct.Pin = USART2_A2_RX_Pin;
            HAL_GPIO_Init(USART2_A2_RX_GPIO_Port, &GPIO_InitStruct);
        } else {
            GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
            GPIO_InitStruct.Pull = GPIO_NOPULL;
            GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
            GPIO_InitStruct.Alternate = RS485_AF;
            GPIO_InitStruct.Pin = RS485_A2_TX_Pin;
            HAL_GPIO_Init(RS485_A2_TX_GPIO_Port, &GPIO_InitStruct);
            GPIO_InitStruct.Pin = RS485_A2_RX_Pin;
            HAL_GPIO_Init(RS485_A2_RX_GPIO_Port, &GPIO_InitStruct);
            GPIO_InitStruct.Pin = RS485_12_DE_Pin;
            HAL_GPIO_Init(RS485_A1_DE_GPIO_Port, &GPIO_InitStruct);
        }

        // USART2_RX Init
        hdma_usart2_rx.Instance = USART2_RX_DMA_Channel;
        hdma_usart2_rx.Init.Request = DMA_REQUEST_2;
        hdma_usart2_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_usart2_rx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_usart2_rx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_usart2_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_usart2_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_usart2_rx.Init.Mode = DMA_NORMAL;
        hdma_usart2_rx.Init.Priority = DMA_PRIORITY_LOW;
        if (HAL_DMA_Init(&hdma_usart2_rx) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(uartHandle,hdmarx,hdma_usart2_rx);

        // USART2_TX Init
        hdma_usart2_tx.Instance = USART2_TX_DMA_Channel;
        hdma_usart2_tx.Init.Request = DMA_REQUEST_2;
        hdma_usart2_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_usart2_tx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_usart2_tx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_usart2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_usart2_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_usart2_tx.Init.Mode = DMA_NORMAL;
        hdma_usart2_tx.Init.Priority = DMA_PRIORITY_LOW;
        if (HAL_DMA_Init(&hdma_usart2_tx) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(uartHandle,hdmatx,hdma_usart2_tx);

        // USART2 interrupt Init
        HAL_NVIC_SetPriority(USART2_IRQn, INTERRUPT_PRIO_SERIAL, 0);
        HAL_NVIC_EnableIRQ(USART2_IRQn);
        HAL_NVIC_SetPriority(USART2_RX_DMA_Channel, INTERRUPT_PRIO_SERIAL, 0);
        HAL_NVIC_EnableIRQ(USART2_RX_DMA_Channel);
        HAL_NVIC_SetPriority(USART2_TX_DMA_Channel, INTERRUPT_PRIO_SERIAL, 0);
        HAL_NVIC_EnableIRQ(USART2_TX_DMA_Channel);

    }

}

// UART msp deinit
void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

    if (uartHandle->Instance==LPUART1) {

        // Peripheral clock disable
        __HAL_RCC_LPUART1_CLK_DISABLE();

        // Deinit GPIOs
        if (!lpuart1UsingAlternatePins) {
            HAL_GPIO_DeInit(LPUART1_VCP_RX_GPIO_Port, LPUART1_VCP_RX_Pin);
            HAL_GPIO_DeInit(LPUART1_VCP_TX_GPIO_Port, LPUART1_VCP_TX_Pin);
        } else {
            HAL_GPIO_DeInit(LPUART1_A3_RX_GPIO_Port, LPUART1_A3_RX_Pin);
            HAL_GPIO_DeInit(LPUART1_A2_TX_GPIO_Port, LPUART1_A2_TX_Pin);
        }

        // LPUART1 interrupt Deinit
        HAL_NVIC_DisableIRQ(LPUART1_IRQn);

    }

    if (uartHandle->Instance==USART1) {

        // Peripheral clock disable
        __HAL_RCC_USART1_CLK_DISABLE();

        //  GPIO Configuration
        HAL_GPIO_DeInit(USART1_TX_GPIO_Port, USART1_TX_Pin);
        HAL_GPIO_DeInit(USART1_RX_GPIO_Port, USART1_RX_Pin);

        // USART1 DMA DeInit
        HAL_DMA_DeInit(uartHandle->hdmarx);
        HAL_DMA_DeInit(uartHandle->hdmatx);

        // USART1 interrupt Deinit
        HAL_NVIC_DisableIRQ(USART1_IRQn);
        HAL_NVIC_DisableIRQ(USART1_RX_DMA_IRQn);
        HAL_NVIC_DisableIRQ(USART1_TX_DMA_IRQn);

    }

    if(uartHandle->Instance==USART2) {

        // Peripheral clock disable
        __HAL_RCC_USART2_CLK_DISABLE();

        // GPIO Configuration
        if (!usart2UsingRS485) {
            HAL_GPIO_DeInit(USART2_A2_TX_GPIO_Port, USART2_A2_TX_Pin);
            HAL_GPIO_DeInit(USART2_A2_RX_GPIO_Port, USART2_A2_RX_Pin);
        } else {
            HAL_GPIO_DeInit(RS485_A2_TX_GPIO_Port, RS485_A2_TX_Pin);
            HAL_GPIO_DeInit(RS485_A2_RX_GPIO_Port, RS485_A2_RX_Pin);
            HAL_GPIO_DeInit(RS485_A1_DE_GPIO_Port, RS485_12_DE_Pin);
        }

        // USART2 DMA DeInit
        HAL_DMA_DeInit(uartHandle->hdmarx);
        HAL_DMA_DeInit(uartHandle->hdmatx);

        // USART2 interrupt Deinit
        HAL_NVIC_DisableIRQ(USART2_IRQn);
        HAL_NVIC_DisableIRQ(USART2_RX_DMA_Channel);
        HAL_NVIC_DisableIRQ(USART2_TX_DMA_Channel);

    }

}
