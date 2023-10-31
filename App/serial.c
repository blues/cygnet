
#include "app.h"
#include "serial.h"
#include "usart.h"

// This set of methods has two jobs:
// 1. rapidly transfer data from interrupt buffers into userspace buffers without loss
// 2. gather non-blank lines that are terminated by either \r or \n and wake up req task to process them

// Port descriptors
typedef struct {
    array *bytes;
    bool bytesTerminated;
    bool swallowNextNewline;
    mutex rxLock;
    mutex txLock;
} serialDesc;
STATIC serialDesc usart1Desc = {0};
STATIC serialDesc usart2Desc = {0};
STATIC serialDesc lpuart1Desc = {0};

// Interrupt buffers
STATIC uint8_t lpuart1InterruptBuffer[600];
STATIC uint8_t usart1InterruptBuffer[600];
STATIC uint8_t usart2InterruptBuffer[600];
STATIC uint8_t isrDebugOutput[120];             // some messages will get truncated but who cares
STATIC uint32_t isrDebugOutputLen = 0;

// Last time we did work moving serial data
STATIC int64_t lastTimeDidWorkMs = 0L;

// Task ID
STATIC uint32_t serialTaskID = TASKID_UNKNOWN;
STATIC uint32_t reqTaskID = TASKID_UNKNOWN;

// Reset the USB appropriate
STATIC bool resetUSB = false;

// Forwards
void serialReceivedNotification(UART_HandleTypeDef *huart, bool error);
bool pollPort(UART_HandleTypeDef *huart);

// Notify that the USB port needs to be reset
void serialResetUSB()
{
    resetUSB = true;
}

// Serial poller init
void serialInit(uint32_t taskID)
{

    // Remember our task ID
    serialTaskID = taskID;

    // Init the mutexes
    mutexInit(&lpuart1Desc.rxLock, MTX_SERIAL_RX);
    mutexInit(&lpuart1Desc.txLock, MTX_SERIAL_TX);
    mutexInit(&usart1Desc.rxLock, MTX_SERIAL_RX);
    mutexInit(&usart1Desc.txLock, MTX_SERIAL_TX);
    mutexInit(&usart2Desc.rxLock, MTX_SERIAL_RX);
    mutexInit(&usart2Desc.txLock, MTX_SERIAL_TX);

    // LPUART1 (notecard request port)
    MX_UART_RxConfigure(&hlpuart1, lpuart1InterruptBuffer, sizeof(lpuart1InterruptBuffer), serialReceivedNotification);
    MX_LPUART1_UART_Init(false, 9600);

    // USART1 (debug port)
    MX_UART_RxConfigure(&huart1, usart1InterruptBuffer, sizeof(usart1InterruptBuffer), serialReceivedNotification);
    MX_USART1_UART_Init(115200);
    MX_DBG_SetOutput(&huart1, serialOutput);

    // USART2 (modem port)
    MX_UART_RxConfigure(&huart2, usart2InterruptBuffer, sizeof(usart2InterruptBuffer), serialReceivedNotification);
    MX_USART2_UART_Init(false, 9600);

}

// Serial poller
void serialPoll(void)
{

    // Rest the USB if it needs to be reset
    if (resetUSB) {
        resetUSB = false;
        if ((peripherals & PERIPHERAL_USART1) != 0) {
            if (osUsbDetected()) {
                MX_USART1_UART_ReInit();
            } else {
                MX_USART1_UART_DeInit();
            }
        }
    }

    // Poll ports so long as there's something to do
    bool didWork = false;
    while (true) {
        bool didSomething = false;
        didSomething |= pollPort(&hlpuart1);
        didSomething |= pollPort(&huart1);
        didSomething |= pollPort(&huart2);
        if (isrDebugOutputLen > 0) {
            didSomething = true;
            serialOutput(&huart1, NULL, 0);
        }
        didWork |= didSomething;
        if (!didSomething) {
            break;
        }
    }

    // If nothing was done, truly go idle
    uint32_t pollMs = ms1Hour;
    if (didWork) {
        pollMs = 1;
        lastTimeDidWorkMs = timerMs();
    }

    // Wait until there's something to do
    taskTake(serialTaskID, pollMs);

}

// Notification
void serialReceivedNotification(UART_HandleTypeDef *huart, bool error)
{
    if (serialTaskID != TASKID_UNKNOWN) {
        taskGiveFromISR(serialTaskID);
    }
}

// Get the descriptor for the port
serialDesc *portDesc(UART_HandleTypeDef *huart)
{
    if (huart == &huart1) {
        return &usart1Desc;
    } else if (huart == &huart2) {
        return &usart2Desc;
    } else if (huart == &hlpuart1) {
        return &lpuart1Desc;
    }
    return NULL;
}

// See if there's port activity
bool pollPort(UART_HandleTypeDef *huart)
{

    // Get the descriptor
    serialDesc *desc = portDesc(huart);
    if (desc == NULL) {
        return false;
    }

    // Exit immediately if nothing available
    if (!HAL_UART_RxAvailable(huart)) {
        return false;
    }

    // Lock the mutex and see if we're already processing something.  This
    // is a really unusual condition because it means that someone is
    // sequentially chaining "cmd"s or something like that.  We need
    // to wait until that command is processed, though, before taking
    // any more from the interrupt handler.
    mutexLock(&desc->rxLock);
    if (desc->bytesTerminated) {
        mutexUnlock(&desc->rxLock);
        taskGive(reqTaskID);
        timerMsSleep(50);
        return true;
    }

    // Get the databyte
    uint8_t databyte = HAL_UART_RxGet(huart);

    // Alloc if new
    if (desc->bytes == NULL) {
        if (arrayAllocBytes(&desc->bytes) != errNone) {
            mutexUnlock(&desc->rxLock);
            return false;
        }
        desc->bytesTerminated = false;
    }

    // Swallow newline if appropriate
    if (databyte == '\n' && desc->swallowNextNewline) {
        desc->swallowNextNewline = false;
        mutexUnlock(&desc->rxLock);
        return true;
    }

    // Awaken request processing task if a control character, because it's a waste to do otherwise
    if (databyte == '\r' || databyte == '\n') {
        desc->swallowNextNewline = (databyte == '\r');
        if (reqTaskID != TASKID_UNKNOWN) {
            desc->bytesTerminated = true;
            taskGive(reqTaskID);
        }
        mutexUnlock(&desc->rxLock);
        return true;
    }
    desc->swallowNextNewline = false;

    // Append all bytes except \r and \n, making sure that we ALWAYS have a '\0' at the end
    // so that later we can do a JParse that requires a null-terminated string.
    char byteString[2];
    byteString[0] = (char) databyte;
    byteString[1] = '\0';
    if (arrayAppendStringBytes(desc->bytes, byteString) != errNone) {
        mutexUnlock(&desc->rxLock);
        return false;
    }

    // Done
    mutexUnlock(&desc->rxLock);
    return true;
}

// Set the task ID of the request task
void serialSetReqTaskID(uint32_t taskID)
{
    reqTaskID = taskID;
}

// See if there's data, and lock the receive buffer if so
bool serialLock(UART_HandleTypeDef *huart, uint8_t **retData, uint32_t *retDataLen, bool *retDiagAllowed)
{

    // Get port desc
    serialDesc *desc = portDesc(huart);
    if (desc == NULL) {
        return false;
    }

    // Get data
    if (desc->bytes == NULL) {
        return false;
    }
    mutexLock(&desc->rxLock);
    if (desc->bytes == NULL) {
        mutexUnlock(&desc->rxLock);
        return false;
    }

    // Return buffer, leaving desc locked.  Note that we return
    // with 0-length if just a newline was passed to us, and this
    // is essential when note-c is initializing and it's just
    // sending \n\n\n's to see if we're alive.
    if (!desc->bytesTerminated) {
        mutexUnlock(&desc->rxLock);
        return false;
    }
    *retData = (uint8_t *) arrayAddress(desc->bytes);
    *retDataLen = arrayLength(desc->bytes);
    *retDiagAllowed = (huart == &huart1);
    return true;

}

// Unlock and potentially reset
void serialUnlock(UART_HandleTypeDef *huart, bool reset)
{

    // Get port desc
    serialDesc *desc = portDesc(huart);
    if (desc == NULL) {
        return;
    }

    // Reset if desired
    if (reset) {
        if (desc->bytes != NULL) {
            arrayFree(desc->bytes);
            desc->bytes = NULL;
        }
        desc->bytesTerminated = false;
    }

    // Unlock
    mutexUnlock(&desc->rxLock);
}

// Output string to debug uart
void serialOutputString(UART_HandleTypeDef *huart, char *buf)
{
    serialOutput(huart, (uint8_t *)buf, strlen(buf));
}

// Output to debug uart
void serialOutput(UART_HandleTypeDef *huart, uint8_t *buf, uint32_t buflen)
{

    // Buffer the output if we're in an ISR (which ST's middleware does)
    if (MX_InISR()) {
        uint32_t left = sizeof(isrDebugOutput)-isrDebugOutputLen;
        if (buflen > left) {
            buflen = left;
        }
        memcpy(&isrDebugOutput[isrDebugOutputLen], buf, buflen);
        isrDebugOutputLen += buflen;
        taskGiveFromISR(serialTaskID);
        return;
    }

    // Output
    serialDesc *desc = portDesc(huart);
    if (desc == NULL) {
        return;
    }
    mutexLock(&desc->txLock);
    if (isrDebugOutputLen > 0) {
        MX_UART_Transmit(huart, isrDebugOutput, isrDebugOutputLen, 500);
        isrDebugOutputLen = 0;
    }
    if (buflen > 0) {
        MX_UART_Transmit(huart, buf, buflen, 500);
    }
    mutexUnlock(&desc->txLock);
}

// Output to the specified port with the Request Terminator (\r\n)
void serialOutputLn(UART_HandleTypeDef *huart, uint8_t *buf, uint32_t buflen)
{
    serialDesc *desc = portDesc(huart);
    if (desc == NULL) {
        return;
    }
    mutexLock(&desc->txLock);
    if (buflen == 0) {
        MX_UART_Transmit(huart, "\r\n", 2, 500);
    } else {
        // We prefer to send it in a single chunk because it eliminates
        // an I2C poll iteration for the client.
        uint8_t *temp;
        err_t err = memDup(buf, buflen+2, &temp);
        if (err) {
            MX_UART_Transmit(huart, buf, buflen, 500);
            MX_UART_Transmit(huart, "\r\n", 2, 500);
        } else {
            temp[buflen++] = '\r';
            temp[buflen++] = '\n';
            MX_UART_Transmit(huart, temp, buflen, 500);
            memFree(temp);
        }
    }
    mutexUnlock(&desc->txLock);
}

// Output an object to the specified port, and free it, also outputting to the debug console if appropriate
void serialOutputObject(UART_HandleTypeDef *huart, J *msg)
{
    if (msg == NULL) {
        return;
    }
    ledBusy(true);
    char *json = JPrintUnformattedOmitEmpty(msg);
    JDelete(msg);
    if (huart != &huart1) {
        debugMessage("<< ");
        debugMessage(json);
        debugMessage("\n");
    }
    serialOutputLn(huart, (uint8_t *)json, strlen(json));
    memFree(json);
    ledBusy(false);
}

// Output an object to the specified port, and free it, also outputting to the debug console if appropriate
void serialSendMessageToNotecard(J *msg)
{
    serialOutputObject(&hlpuart1, msg);
}

// Create an outgoing message object intended to be sent to the notecard
J *serialCreateMessage(const char *msgType, J *body, uint8_t *payload, uint32_t payloadLen)
{
    J *msg = JCreateObject();
    JAddStringToObject(msg, "cmd", "card.ntn");
    JDeleteItemFromObject(body, FieldMsgType);
    JAddStringToObject(body, FieldMsgType, MsgHello);
    JAddItemToObject(msg, "body", body);
    if (payload != NULL && payloadLen != 0) {
        JAddBinaryToObject(msg, "payload", payload, payloadLen);
    }
    return msg;
}
