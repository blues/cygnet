// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "stm32l4xx_ll_lpuart.h"
#include "stm32l4xx_hal_uart.h"
#include "stm32l4xx_hal_uart_ex.h"
#include "stm32l4xx_hal_usart_ex.h"
#include "main.h"
#include "usart.h"
#include "usb_device.h"
#include "global.h"
#include "dma.h"
#include <stdatomic.h>

UART_HandleTypeDef hlpuart1;
bool lpuart1UsingAlternatePins = false;
uint32_t lpuart1BaudRate = 0;

UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart1_tx;
uint32_t usart1BaudRate = 0;

UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_rx;
DMA_HandleTypeDef hdma_usart2_tx;
bool usart2UsingRS485 = false;
uint32_t usart2BaudRate = 0;

// LPUART variable speed handling
#if defined(LPUART1_DISABLE_HIGH_BUSY_SAMPLING_RATE)
uint32_t lpuart1PeriphClockSelection = RCC_LPUART1CLKSOURCE_LSE;
#else
uint32_t lpuart1PeriphClockSelection = RCC_LPUART1CLKSOURCE_HSI;
#endif

// For UART receive
// UART receive I/O descriptor.  Note that if we ever fill the buffer
// completely we will get an I/O error because we can't start a receive
// without overwriting the data waiting to be pulled out.  There must
// always be at least one byte available to start a receive.
typedef struct {
    uint8_t *iobuf;
    uint16_t iobuflen;
    uint8_t *buf;
    uint16_t buflen;
    uint16_t fill;
    uint16_t drain;
    uint16_t rxlen;
    void (*notifyReceivedFn)(UART_HandleTypeDef *huart, bool error);
} UARTIO;
UARTIO rxioLPUART1 = {0};
UARTIO rxioUSART1 = {0};
UARTIO rxioUSART2 = {0};
UARTIO rxioUSB = {0};

// Number of bytes for UART receives, and a temporary RXBUF for double-buffering
#define UART_IOBUF_LEN 512
atomic_int rxtempInUse = 0;
uint8_t rxtemp[UART_IOBUF_LEN];

// Forwards
bool uioReceivedBytes(UARTIO *uio, uint8_t *buf, uint32_t buflen);
void receiveComplete(UART_HandleTypeDef *huart, UARTIO *uio, uint8_t *buf, uint32_t buflen);

// See if a port is DMA
bool MX_UART_IsDMA(UART_HandleTypeDef *huart)
{
    if (huart == &huart1) {
#if USART1_USE_DMA
        return true;
#else
        return false;
#endif
    }
    if (huart == &huart2) {
#if USART2_USE_DMA
        return true;
#else
        return false;
#endif
    }
    return false;
}

// Transmit to a port synchronously, broken up into 64 byte chunks because we have
// seen repeatedly that hosts are not generally designed to handle large transfers.
// We've chosen 64 bytes simply due to the fact that USB frame size is 64.
void MX_UART_Transmit(UART_HandleTypeDef *huart, uint8_t *buf, uint32_t len, uint32_t timeoutMs)
{
    uint32_t chunksize = 64;
    while (len) {

        // Break it into chunks
        uint32_t chunklen = len;
        if (chunklen > chunksize) {
            chunklen = chunksize;
        }

        // Wait up to 100mS for transmission of a single chunk to succeed
        uint32_t singleChunkTimeoutMs = 100;
        for (int i=0;; i++) {
            if (i >= singleChunkTimeoutMs) {
                return;
            }
            if (MX_UART_TransmitFull(huart, buf, chunklen, timeoutMs)) {
                break;
            }
            HAL_Delay(1);
        }

        // Delay a bit between chunks
        uint32_t delayBetweenChunksMs = 10;
        HAL_Delay(delayBetweenChunksMs);
        buf += chunklen;
        len -= chunklen;

    }
}


// Transmit a 64-byte chunk to a port, returning false if busy or failure, else true if success
bool MX_UART_TransmitFull(UART_HandleTypeDef *huart, uint8_t *buf, uint32_t len, uint32_t timeoutMs)
{

    // USB is a special case but the same logic
    if (huart == NULL) {
        if (CDC_Transmit_Completed() == USBD_BUSY) {
            return false;
        }
        bool success = (CDC_Transmit_FS(buf, len) == USBD_OK);
        for (uint32_t i=0; success; i++) {
            if (CDC_Transmit_Completed() != USBD_BUSY) {
                break;
            }
            if (i >= timeoutMs) {
                return false;
            }
            HAL_Delay(1);
        }
        return success;
    }

    // Exit if busy
    if ((HAL_UART_GetState(huart) & HAL_UART_STATE_BUSY_TX) == HAL_UART_STATE_BUSY_TX) {
        return false;
    }

    // Transmit
    bool success = false;
    if (huart == &hlpuart1) {
        success = (HAL_UART_Transmit_IT(huart, buf, len) == HAL_OK);
    } else if (huart == &huart1) {
#if USART1_USE_DMA
        success = (HAL_UART_Transmit_DMA(huart, buf, len) == HAL_OK);
#else
        success = (HAL_UART_Transmit_IT(huart, buf, len) == HAL_OK);
#endif
    } else if (huart == &huart2) {
#if USART2_USE_DMA
        success = (HAL_UART_Transmit_DMA(huart, buf, len) == HAL_OK);
#else
        success = (HAL_UART_Transmit_IT(huart, buf, len) == HAL_OK);
#endif
    }

    // Wait, so that the caller won't mess with the buffer while the HAL is using it
    for (uint32_t i=0; success; i++) {
        if ((HAL_UART_GetState(huart) & HAL_UART_STATE_BUSY_TX) != HAL_UART_STATE_BUSY_TX) {
            break;
        }
        if (i >= timeoutMs) {
            return false;
        }
        HAL_Delay(1);
    }

    // Success
    return success;

}

// Transmit complete callback for serial ports
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
}

// We must restart the receive if there is a receive or transmit error
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    HAL_UART_RxCpltCallback(huart);
}

// Get rx port
UARTIO *rxPort(UART_HandleTypeDef *huart, uint16_t *rxBytes)
{
    UARTIO *uio = NULL;
    uint16_t receivedBytes = 0;
    if (huart == NULL) {
        uio = &rxioUSB;
        receivedBytes = 0;
    }
    if (huart == &hlpuart1) {
        uio = &rxioLPUART1;
        receivedBytes = uio->iobuflen - huart->RxXferCount;
    }
    if (huart == &huart1) {
        uio = &rxioUSART1;
#if USART1_USE_DMA
        receivedBytes = uio->iobuflen - __HAL_DMA_GET_COUNTER(huart->hdmarx);
#else
        receivedBytes = uio->iobuflen - huart->RxXferCount;
#endif
    }
    if (huart == &huart2) {
        uio = &rxioUSART2;
#if USART2_USE_DMA
        receivedBytes = uio->iobuflen - __HAL_DMA_GET_COUNTER(huart->hdmarx);
#else
        receivedBytes = uio->iobuflen - huart->RxXferCount;
#endif
    }
    *rxBytes = receivedBytes;
    return uio;
}

// UART IRQ handler, used exclusively for IDLE processing
void MX_UART_IDLE_IRQHandler(UART_HandleTypeDef *huart)
{
    if (__HAL_UART_GET_IT_SOURCE(huart, UART_IT_IDLE) != RESET && __HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE) != RESET) {

        // Clear the idle flag.
        __HAL_UART_CLEAR_IDLEFLAG(huart);

        // Get the receive port and received bytes
        uint16_t receivedBytes;
        UARTIO *uio = rxPort(huart, &receivedBytes);
        if (uio == NULL) {
            return;
        }

        // Only process things that won't naturally go to a RxCpltCallback
        if (receivedBytes != 0 && receivedBytes != uio->iobuflen) {
            HAL_UART_RxCpltCallback(huart);
        }

    }
}

// Start a receive
void MX_UART_RxStart(UART_HandleTypeDef *huart)
{

    // Exit if this is USB
    if (huart == NULL) {
        return;
    }

    // Importantly, abort any existing transfer so the receive doesn't return BUSY, such as
    // is the case when restarting receive after an IDLE interrupt.
    if (huart->RxState != HAL_UART_STATE_READY) {
        HAL_UART_AbortReceive(huart);
    }

    // Start the new receive
    if (huart == &hlpuart1 && rxioLPUART1.buf != NULL) {
        if (HAL_UART_Receive_IT(huart, rxioLPUART1.iobuf, rxioLPUART1.iobuflen) == HAL_OK) {
            return;
        }
        return;
    }
    if (huart == &huart1 && rxioUSART1.buf != NULL) {
#if USART1_USE_DMA
        if (HAL_UART_Receive_DMA(huart, rxioUSART1.iobuf, rxioUSART1.iobuflen) == HAL_OK) {
            return;
        }
#else
        if (HAL_UART_Receive_IT(huart, rxioUSART1.iobuf, rxioUSART1.iobuflen) == HAL_OK) {
            return;
        }
#endif
        return;
    }
    if (huart == &huart2 && rxioUSART2.buf != NULL) {
#if USART2_USE_DMA
        if (HAL_UART_Receive_DMA(huart, rxioUSART2.iobuf, rxioUSART2.iobuflen) == HAL_OK) {
            return;
        }
#else
        if (HAL_UART_Receive_IT(huart, rxioUSART2.iobuf, rxioUSART2.iobuflen) == HAL_OK) {
            return;
        }
#endif
        return;
    }

}

// Register a completion callback and allocate receive buffer
void MX_UART_RxConfigure(UART_HandleTypeDef *huart, uint8_t *rxbuf, uint16_t rxbuflen, void (*cb)(UART_HandleTypeDef *huart, bool error))
{

    // Initialize receive buffers
    if (huart == NULL) {
        rxioUSB.buf = rxbuf;
        rxioUSB.buflen = rxbuflen;
        rxioUSB.fill = rxioUSB.drain = rxioUSB.rxlen = 0;
        rxioUSB.notifyReceivedFn = cb;
        rxioUSB.iobuflen = 0;
        rxioUSB.iobuf = NULL;
    }
    if (huart == &hlpuart1) {
        rxioLPUART1.buf = rxbuf;
        rxioLPUART1.buflen = rxbuflen;
        rxioLPUART1.fill = rxioLPUART1.drain = rxioLPUART1.rxlen = 0;
        rxioLPUART1.notifyReceivedFn = cb;
        rxioLPUART1.iobuflen = UART_IOBUF_LEN;
        err_t err = memAlloc(rxioLPUART1.iobuflen, &rxioLPUART1.iobuf);
        if (err) {
            debugPanic("lpuart1 iobuf");
        }
    }
    if (huart == &huart1) {
        rxioUSART1.buf = rxbuf;
        rxioUSART1.buflen = rxbuflen;
        rxioUSART1.fill = rxioUSART1.drain = rxioUSART1.rxlen =  0;
        rxioUSART1.notifyReceivedFn = cb;
        rxioUSART1.iobuflen = UART_IOBUF_LEN;
        err_t err = memAlloc(rxioUSART1.iobuflen, &rxioUSART1.iobuf);
        if (err) {
            debugPanic("usart1 iobuf");
        }
    }
    if (huart == &huart2) {
        rxioUSART2.buf = rxbuf;
        rxioUSART2.buflen = rxbuflen;
        rxioUSART2.fill = rxioUSART2.drain = rxioUSART2.rxlen =  0;
        rxioUSART2.notifyReceivedFn = cb;
        rxioUSART2.iobuflen = UART_IOBUF_LEN;
        err_t err = memAlloc(rxioUSART2.iobuflen, &rxioUSART2.iobuf);
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

// Receive complete for USB serial device
void MX_USB_RxCplt(uint8_t* buf, uint32_t buflen)
{
    receiveComplete(NULL, &rxioUSB, buf, buflen);
}

// Receive complete
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{

    // If we've gone into low speed mode, crank it back up
#if !defined(LPUART1_DISABLE_HIGH_BUSY_SAMPLING_RATE)
    if (huart == &hlpuart1 && lpuart1PeriphClockSelection != RCC_LPUART1CLKSOURCE_HSI) {
        lpuart1PeriphClockSelection = RCC_LPUART1CLKSOURCE_HSI;
        __HAL_RCC_LPUART1_CONFIG(lpuart1PeriphClockSelection);
        hlpuart1.Instance->BRR = UART_DIV_LPUART(HSI_VALUE, hlpuart1.Init.BaudRate);
    }
#endif

    // Get the receive port
    uint16_t receivedBytes;
    UARTIO *uio = rxPort(huart, &receivedBytes);
    if (uio == NULL) {
        return;
    }

    // Process completed data
    receiveComplete(huart, uio, uio->iobuf, receivedBytes);

}

// Receive complete
void receiveComplete(UART_HandleTypeDef *huart, UARTIO *uio, uint8_t *buf, uint32_t buflen)
{

    // If there's an error, abort
    if (huart != NULL && huart->ErrorCode != HAL_UART_ERROR_NONE) {
        HAL_UART_Abort(huart);
        uio->fill = uio->drain = uio->rxlen = 0;
        if (uio->notifyReceivedFn != NULL) {
            uio->notifyReceivedFn(huart, true);
        }
        MX_UART_RxStart(huart);
        return;
    }

    // Use the static buffer and do double-buffering if possible.  Note that
    // this will almost always be the case, unless the interrupt priorities
    // are changed such that one serial IRQ is higher prio than the others.
    uint8_t *iobuf = buf;
    if (atomic_fetch_add(&rxtempInUse, 1) == 0) {
        memcpy(rxtemp, iobuf, buflen);
        iobuf = rxtemp;
        MX_UART_RxStart(huart);
    }

    // Process the received bytes
    if (!uioReceivedBytes(uio, iobuf, buflen)) {
        if (uio->notifyReceivedFn != NULL) {
            uio->notifyReceivedFn(huart, true);
        }
    } else {
        if (uio->notifyReceivedFn != NULL) {
            uio->notifyReceivedFn(huart, false);
        }
    }

    // Release the double-buffer or restart the receive
    if (iobuf != rxtemp) {
        MX_UART_RxStart(huart);
    }
    atomic_fetch_sub(&rxtempInUse, 1);

}

// See if anything is available
bool MX_UART_RxAvailable(UART_HandleTypeDef *huart)
{
    UARTIO *uio;
    if (huart == NULL) {
        uio = &rxioUSB;
    }
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
uint8_t MX_UART_RxGet(UART_HandleTypeDef *huart)
{
    UARTIO *uio;
    if (huart == NULL) {
        uio = &rxioUSB;
    }
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

// LPUART1 init function
void MX_LPUART1_UART_Init(bool altPins, uint32_t baudRate)
{
    lpuart1UsingAlternatePins = altPins;
    lpuart1BaudRate = baudRate;
    MX_LPUART1_UART_ReInit();
}

// LPUART1 reinit function
void MX_LPUART1_UART_ReInit(void)
{

    hlpuart1.Instance = LPUART1;
    hlpuart1.Init.BaudRate = lpuart1BaudRate;
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

    __HAL_UART_ENABLE_IT(&hlpuart1, UART_IT_IDLE);

    MX_UART_RxStart(&hlpuart1);

    peripherals |= PERIPHERAL_LPUART1;

}

// LPUART1 suspend function
void MX_LPUART1_UART_Suspend(void)
{

    // Exit if not enabled
    if ((peripherals & PERIPHERAL_LPUART1) == 0) {
        return;
    }

        // Before going to sleep, make sure the LPUART is in low speed mode.
#if !defined(LPUART1_DISABLE_HIGH_BUSY_SAMPLING_RATE)
    if (lpuart1PeriphClockSelection != RCC_LPUART1CLKSOURCE_LSE) {
        lpuart1PeriphClockSelection = RCC_LPUART1CLKSOURCE_LSE;
        __HAL_RCC_LPUART1_CONFIG(lpuart1PeriphClockSelection);
        hlpuart1.Instance->BRR = UART_DIV_LPUART(LSE_VALUE, hlpuart1.Init.BaudRate);
    }
#endif

    // Set wakeUp event on start bit
    UART_WakeUpTypeDef WakeUpSelection;
    WakeUpSelection.WakeUpEvent = LL_LPUART_WAKEUP_ON_RXNE;
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

    // Exit if not enabled
    if ((peripherals & PERIPHERAL_LPUART1) == 0) {
        return;
    }

    // Notify of activity, so that we stay awake
    if (rxioLPUART1.notifyReceivedFn != NULL) {
        rxioLPUART1.notifyReceivedFn(&hlpuart1, true);
    }

}

// Transmit to LPUART1
void MX_LPUART1_UART_Transmit(uint8_t *buf, uint32_t len, uint32_t timeoutMs)
{

    // Exit if not enabled
    if ((peripherals & PERIPHERAL_LPUART1) == 0) {
        return;
    }

    // If we've gone into low speed mode, crank it back up
#if !defined(LPUART1_DISABLE_HIGH_BUSY_SAMPLING_RATE)
    if (lpuart1PeriphClockSelection != RCC_LPUART1CLKSOURCE_HSI) {
        lpuart1PeriphClockSelection = RCC_LPUART1CLKSOURCE_HSI;
        __HAL_RCC_LPUART1_CONFIG(lpuart1PeriphClockSelection);
        hlpuart1.Instance->BRR = UART_DIV_LPUART(HSI_VALUE, hlpuart1.Init.BaudRate);
    }
#endif

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
    __HAL_UART_DISABLE_IT(&hlpuart1, UART_IT_IDLE);
    rxioLPUART1.fill = rxioLPUART1.drain = 0;
    __HAL_RCC_USART1_FORCE_RESET();
    __HAL_RCC_USART1_RELEASE_RESET();
    HAL_UART_DeInit(&hlpuart1);
}

// USART1 init function
void MX_USART1_UART_Init(uint32_t baudRate)
{
    usart1BaudRate = baudRate;
    MX_USART1_UART_ReInit();
}

// USART1 reinit function
void MX_USART1_UART_ReInit(void)
{

    huart1.Instance = USART1;
    huart1.Init.BaudRate = usart1BaudRate;
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

    __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);

    MX_UART_RxStart(&huart1);

    peripherals |= PERIPHERAL_USART1;

}

// USART1 Deinitialization
void MX_USART1_UART_DeInit(void)
{

    // Deinitialized
    peripherals &= ~PERIPHERAL_USART1;

    // Stop idle interrupt
    __HAL_UART_DISABLE_IT(&huart1, UART_IT_IDLE);

    // Deconfigure RX buffer
    rxioUSART1.fill = rxioUSART1.drain = 0;

    // Stop any pending DMA, if any
#if USART1_USE_DMA
    HAL_UART_DMAStop(&huart1);
    HAL_NVIC_DisableIRQ(USART1_RX_DMA_IRQn);
    HAL_NVIC_DisableIRQ(USART1_TX_DMA_IRQn);
#endif

    // Reset peripheral
    __HAL_RCC_USART1_FORCE_RESET();
    __HAL_RCC_USART1_RELEASE_RESET();

    // Deinit
    HAL_UART_DeInit(&huart1);

}

// USART2 init function
void MX_USART2_UART_Init(bool useRS485, uint32_t baudRate)
{
    usart2UsingRS485 = useRS485;
    usart2BaudRate = baudRate;
    MX_USART2_UART_ReInit();
}

// USART2 reinit function
void MX_USART2_UART_ReInit(void)
{

    huart2.Instance = USART2;
    huart2.Init.BaudRate = usart2BaudRate;
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

    __HAL_UART_ENABLE_IT(&huart2, UART_IT_IDLE);

    MX_UART_RxStart(&huart2);

    peripherals |= PERIPHERAL_USART2;

}

// USART2 Deinitialization
void MX_USART2_UART_DeInit(void)
{

    // Deinitialized
    peripherals &= ~PERIPHERAL_USART2;

    // Stop idle interrupt
    __HAL_UART_DISABLE_IT(&huart2, UART_IT_IDLE);

    // Deconfigure RX buffer
    rxioUSART2.fill = rxioUSART2.drain = 0;

    // Stop any pending DMA and disable DMA intnerrupts
#if USART2_USE_DMA
    HAL_UART_DMAStop(&huart2);
    HAL_NVIC_DisableIRQ(USART2_RX_DMA_IRQn);
    HAL_NVIC_DisableIRQ(USART2_TX_DMA_IRQn);
#endif

    // Reset peripheral
    __HAL_RCC_USART2_FORCE_RESET();
    __HAL_RCC_USART2_RELEASE_RESET();

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
        PeriphClkInit.Lpuart1ClockSelection = lpuart1PeriphClockSelection;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
            Error_Handler();
        }

        // LPUART1 clock enable
        __HAL_RCC_LPUART1_CLK_ENABLE();

        // GPIO Configuration
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
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

        // LPUART1 interrupt Init.  Note that we use a higher interrupt
        // priority than DMA serial because otherwise we may lose chars
        HAL_NVIC_SetPriority(LPUART1_IRQn, INTERRUPT_PRIO_ISERIAL, 0);
        HAL_NVIC_EnableIRQ(LPUART1_IRQn);

        // Enable LPUART clock in sleep mode
        // Default state of RCC->APB1SMENR2 ia all enabled see RM0453 7.4.26,
        // but we include this call as a reminder that it is needed.
        __HAL_RCC_LPUART1_CLK_SLEEP_ENABLE();

    }

    if (uartHandle->Instance==USART1) {

        // Initializes the peripherals clock
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
        PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
            Error_Handler();
        }

        // USART1 clock enable
#if USART1_USE_DMA
        MX_DMA_Init();
#endif
        __HAL_RCC_USART1_CLK_ENABLE();

        // GPIO Configuration
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = USART1_AF;
        GPIO_InitStruct.Pin = USART1_TX_Pin;
        HAL_GPIO_Init(USART1_TX_GPIO_Port, &GPIO_InitStruct);
        GPIO_InitStruct.Pin = USART1_RX_Pin;
        HAL_GPIO_Init(USART1_RX_GPIO_Port, &GPIO_InitStruct);

        // USART1_RX Init
#if USART1_USE_DMA
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
#endif

        // USART1_TX Init
#if USART1_USE_DMA
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
#endif

        // USART1 interrupt Init
        HAL_NVIC_SetPriority(USART1_IRQn, INTERRUPT_PRIO_SERIAL, 0);
        HAL_NVIC_EnableIRQ(USART1_IRQn);
#if USART1_USE_DMA
        HAL_NVIC_SetPriority(USART1_RX_DMA_IRQn, INTERRUPT_PRIO_SERIAL, 0);
        HAL_NVIC_EnableIRQ(USART1_RX_DMA_IRQn);
        HAL_NVIC_SetPriority(USART1_TX_DMA_IRQn, INTERRUPT_PRIO_SERIAL, 0);
        HAL_NVIC_EnableIRQ(USART1_TX_DMA_IRQn);
#endif

    }

    if (uartHandle->Instance==USART2) {

        // Initializes the peripherals clock
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2;
        PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
            Error_Handler();
        }

        // USART2 clock enable
#if USART2_USE_DMA
        MX_DMA_Init();
#endif
        __HAL_RCC_USART2_CLK_ENABLE();

        // GPIO Configuration
        if (!usart2UsingRS485) {
            GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
            GPIO_InitStruct.Pull = GPIO_PULLUP;
            GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
            GPIO_InitStruct.Alternate = USART2_AF;
            GPIO_InitStruct.Pin = USART2_A2_TX_Pin;
            HAL_GPIO_Init(USART2_A2_TX_GPIO_Port, &GPIO_InitStruct);
            GPIO_InitStruct.Pin = USART2_A3_RX_Pin;
            HAL_GPIO_Init(USART2_A3_RX_GPIO_Port, &GPIO_InitStruct);
        } else {
            GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
            GPIO_InitStruct.Pull = GPIO_PULLUP;
            GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
            GPIO_InitStruct.Alternate = RS485_AF;
            GPIO_InitStruct.Pin = RS485_A2_TX_Pin;
            HAL_GPIO_Init(RS485_A2_TX_GPIO_Port, &GPIO_InitStruct);
            GPIO_InitStruct.Pin = RS485_A3_RX_Pin;
            HAL_GPIO_Init(RS485_A3_RX_GPIO_Port, &GPIO_InitStruct);
            GPIO_InitStruct.Pin = RS485_A1_DE_Pin;
            HAL_GPIO_Init(RS485_A1_DE_GPIO_Port, &GPIO_InitStruct);
        }

        // USART2_RX DMA Init
#if USART2_USE_DMA
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
#endif

        // USART2_TX DMA Init
#if USART2_USE_DMA
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
#endif

        // USART2 interrupt Init
        HAL_NVIC_SetPriority(USART2_IRQn, INTERRUPT_PRIO_SERIAL, 0);
        HAL_NVIC_EnableIRQ(USART2_IRQn);
#if USART2_USE_DMA
        HAL_NVIC_SetPriority(USART2_RX_DMA_IRQn, INTERRUPT_PRIO_SERIAL, 0);
        HAL_NVIC_EnableIRQ(USART2_RX_DMA_IRQn);
        HAL_NVIC_SetPriority(USART2_TX_DMA_IRQn, INTERRUPT_PRIO_SERIAL, 0);
        HAL_NVIC_EnableIRQ(USART2_TX_DMA_IRQn);
#endif

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
#if USART1_USE_DMA
        HAL_DMA_DeInit(uartHandle->hdmarx);
        HAL_DMA_DeInit(uartHandle->hdmatx);
#endif

        // USART1 interrupt Deinit
        HAL_NVIC_DisableIRQ(USART1_IRQn);
#if USART1_USE_DMA
        HAL_NVIC_DisableIRQ(USART1_RX_DMA_IRQn);
        HAL_NVIC_DisableIRQ(USART1_TX_DMA_IRQn);
#endif

    }

    if(uartHandle->Instance==USART2) {

        // Peripheral clock disable
        __HAL_RCC_USART2_CLK_DISABLE();

        // GPIO Configuration
        if (!usart2UsingRS485) {
            HAL_GPIO_DeInit(USART2_A2_TX_GPIO_Port, USART2_A2_TX_Pin);
            HAL_GPIO_DeInit(USART2_A3_RX_GPIO_Port, USART2_A3_RX_Pin);
        } else {
            HAL_GPIO_DeInit(RS485_A2_TX_GPIO_Port, RS485_A2_TX_Pin);
            HAL_GPIO_DeInit(RS485_A3_RX_GPIO_Port, RS485_A3_RX_Pin);
            HAL_GPIO_DeInit(RS485_A1_DE_GPIO_Port, RS485_A1_DE_Pin);
        }

        // USART2 DMA DeInit
#if USART2_USE_DMA
        HAL_DMA_DeInit(uartHandle->hdmarx);
        HAL_DMA_DeInit(uartHandle->hdmatx);
#endif

        // USART2 interrupt Deinit
        HAL_NVIC_DisableIRQ(USART2_IRQn);
#if USART2_USE_DMA
        HAL_NVIC_DisableIRQ(USART2_RX_DMA_IRQn);
        HAL_NVIC_DisableIRQ(USART2_TX_DMA_IRQn);
#endif

    }

}
