
#include <stdarg.h>
#include "app.h"
#include "serial.h"
#include "usart.h"

// Received lines
STATIC mutex modemReceivedLinesMutex = {MTX_MODEM_RECEIVED, {0}};
STATIC arrayString modemReceivedLines = {0};
STATIC int64_t modemReceivedLineMs = {0};

// Forwards
bool processModemResponse(UART_HandleTypeDef *huart);

// Request task
void modemTask(void *params)
{

    // Init task
    taskRegister(TASKID_MODEM, TASKNAME_MODEM, TASKLETTER_MODEM, TASKSTACK_MODEM);

    // Loop, extracting requests from the serial port and processing them
    while (true) {
        bool didSomething = false;
        didSomething |= processModemResponse(&huart2);
        if (!didSomething) {
            taskTake(TASKID_MODEM, ms1Day);
        }
    }

}

// Process incoming serial request
bool processModemResponse(UART_HandleTypeDef *huart)
{

    // Get the pending JSON request
    uint8_t *modemResponse;
    uint32_t modemResponseLen;
    bool diagAllowed;
    if (!serialLock(huart, &modemResponse, &modemResponseLen, &diagAllowed)) {
        return false;
    }

    // If it's a 0-length request, we must output our standard \r\n response
    // because this is critical for note-c to answer "are you there?"
    if (modemResponseLen == 0) {
        serialUnlock(huart, true);
        return true;
    }

    // Add the line to an array of received lines.  Note that this depends upon
    // the fact that serial.c pollPort() uses arrayAppendStringBytes in such a
    // way that the buffer here is guaranteed to be null-terminated.
    mutexLock(&modemReceivedLinesMutex);
    arrayAppend(&modemReceivedLines, modemResponse);
    modemReceivedLineMs = timerMs();
    mutexUnlock(&modemReceivedLinesMutex);

    // Done
    serialUnlock(huart, true);
    return true;

}

// Send a modem command and get its response(s)
err_t modemSend(arrayString *retResults, char *format, ...)
{
    err_t err = errNone;

    // First, discard anything that is pending
    mutexLock(&modemReceivedLinesMutex);
    for (int i=0; i<arrayEntries(&modemReceivedLines); i++) {
        debugf("modem: discarding \"%s\"\n", (char *) arrayEntry(&modemReceivedLines, i));
    }
    arrayReset(&modemReceivedLines);
    modemReceivedLineMs = 0;
    mutexUnlock(&modemReceivedLinesMutex);

    // Reset results
    arrayReset(retResults);

    // Format the request
    if (strchr(format, '%') == NULL) {
        serialSendLineToModem(format);
    } else {
        char *buf;
        uint32_t buflen = 1024;
        err = memAlloc(buflen, &buf);
        if (err) {
            return errF("%s " ERR_IO, errString(err));
        }
        va_list vaArgs;
        va_start(vaArgs, format);
        vsnprintf(buf, buflen, format, vaArgs);
        serialSendLineToModem(buf);
        va_end(vaArgs);
        memFree(buf);
    }

    // Wait until any of the standard terminators, or timeout
    int64_t expiresMs = timerMs() + 2500;
    bool terminatorReceived = false;
    while (!err && !terminatorReceived) {
        if (timerMs() > expiresMs) {
            return errF(ERR_TIMEOUT);
        }
        mutexLock(&modemReceivedLinesMutex);
        if (arrayEntries(&modemReceivedLines) == 0) {
            mutexUnlock(&modemReceivedLinesMutex);
            timerMsSleep(100);
            continue;
        }
        expiresMs = timerMs() + 500;
        for (int i=0; i<arrayEntries(&modemReceivedLines); i++) {
            char *line = arrayEntry(&modemReceivedLines, i);
            if (strEQL(line, "OK")) {
                terminatorReceived = true;
                err = errNone;
                break;
            }
            if (strEQL(line, "ERROR")) {
                err = errF("error " ERR_MODEM_COMMAND);
                break;
            }
            char *errPrefix = "+CME ERROR: ";
            if (memeql(line, errPrefix, strlen(errPrefix))) {
                err = errF("%s " ERR_MODEM_COMMAND, &line[strlen(errPrefix)]);
                break;
            }
            if (memeql(line, "+CME ", 5)) {
                err = errF("%s " ERR_MODEM_COMMAND, line);
                break;
            }
            arrayAppend(retResults, line);
        }
        arrayReset(&modemReceivedLines);
        modemReceivedLineMs = 0;
        mutexUnlock(&modemReceivedLinesMutex);
    }

    // Deallocate if error
    if (err) {
        arrayReset(retResults);
    }

    // Done
    return err;

}
