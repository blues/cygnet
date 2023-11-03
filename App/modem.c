
#include <stdarg.h>
#include "app.h"
#include "serial.h"
#include "usart.h"

// Modem work to do
typedef struct {
    modemWorker worker;
    J *workBody;
    uint8_t *workPayload;
    uint32_t workPayloadLen;
} work_t;
STATIC mutex modemWorkMutex = {MTX_MODEM_WORK, {0}};
STATIC array *work = NULL;

// Received lines
STATIC mutex modemReceivedLinesMutex = {MTX_MODEM_RECEIVED, {0}};
STATIC arrayString modemReceivedLines = {0};
STATIC int64_t modemReceivedLineMs = {0};

// Identity
char modemId[48] = {0};
char modemVersion[48] = {0};

// Modem state
STATIC bool poweredOn = false;

// Forwards
bool processModemSerialIncoming(void);
bool processModemWork(void);

// Request task
void modemTask(void *params)
{

    // Init task
    taskRegister(TASKID_MODEM, TASKNAME_MODEM, TASKLETTER_MODEM, TASKSTACK_MODEM);

    // Enqueue work to be performed at init
    modemEnqueueWork(workInit, NULL, NULL);

    // Loop, extracting requests from the serial port and processing them
    while (true) {
        bool didSomething = false;
        didSomething |= processModemSerialIncoming();
        if (!didSomething) {
            didSomething |= processModemWork();
        }
        if (!didSomething) {
            taskTake(TASKID_MODEM, ms1Day);
        }
    }

}

// Reset a work entry
void modemWorkReset(void *we)
{
    work_t *w = (work_t *) we;
    if (w->workBody != NULL) {
        JDelete(w->workBody);
        w->workBody = NULL;
    }
    if (w->workPayload != NULL) {
        memFree(w->workPayload);
        w->workPayload = NULL;
        w->workPayloadLen = 0;
    }
}

// Enqueue work.  Note that in all cases (even errors) workToDo is freed if supplied,
// and b64Payload is NOT freed.
err_t modemEnqueueWork(modemWorker worker, J *workBody, char *workPayloadB64)
{
    err_t err;
    
    // Allocate the array if it has never yet been allocated
    mutexLock(&modemWorkMutex);
    if (work == NULL) {
        err = arrayAlloc(sizeof(work_t), modemWorkReset, &work);
        if (err) {
            mutexUnlock(&modemWorkMutex);
            if (workBody != NULL) {
                JDelete(workBody);
            }
            return err;
        }
    }
    mutexUnlock(&modemWorkMutex);

    // Append the work
    work_t w = {0};
    w.worker = worker;
    w.workBody = workBody;

    // Allocate the payload if it's supplied.  As a convenience, guarantee
    // that the payload buffer is one larger than needed and null-terminated.
    if (workPayloadB64 != NULL && *workPayloadB64 != '\0') {
        err = memAlloc(JB64DecodeLen(workPayloadB64)+1, &w.workPayload);
        if (err) {
            if (workBody != NULL) {
                JDelete(workBody);
            }
            return err;
        }
        w.workPayloadLen = JB64Decode((char *)w.workPayload, workPayloadB64);
        w.workPayload[w.workPayloadLen] = '\0';
    }

    err = arrayAppend(work, &w);
    if (err) {
        modemWorkReset(&w);
        return err;
    }

    return errNone;
    
}

// Delete the specified work if found
err_t modemRemoveWork(modemWorker worker)
{
    mutexLock(&modemWorkMutex);
    if (work != NULL) {
        bool retry = true;
        while (retry && arrayEntries(work) > 0) {
            retry = false;
            for (int i=0; i<arrayEntries(work); i++) {
                work_t *w = (work_t *) arrayEntry(work, i);
                if (w->worker == worker) {
                    arrayResetEntry(work, i);
                    arrayRemove(work, i);
                    retry = true;
                    break;
                }
            }
        }
    }
    mutexUnlock(&modemWorkMutex);
    return errNone;
}

// Process one unit of pending modem work
bool processModemWork(void)
{
    bool workToDo = false;
    work_t w;
    mutexLock(&modemWorkMutex);
    if (arrayEntries(work) > 0) {
        memcpy(&w, arrayEntry(work, 0), sizeof(w));
        workToDo = true;
        arrayRemove(work, 0);
    }
    mutexUnlock(&modemWorkMutex);
    if (workToDo) {
        err_t err = w.worker(w.workBody, w.workPayload, w.workPayloadLen);
        modemWorkReset(&w);
        if (err) {
            debugf("modem: %s\n", errString(err));
        }
    }
    return workToDo;
}

// Process incoming serial request
bool processModemSerialIncoming(void)
{

    // Modem port
    UART_HandleTypeDef *huart = &huart2;

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

    // Unlock serial
    serialUnlock(huart, true);

    // Trace and exit
    debugR("modem: << %s\n", modemResponse);
    return true;

}

// Send a modem command and get its response(s)
err_t modemSend(arrayString *retResults, char *format, ...)
{
    err_t err = errNone;

    // If we don't want results, use a static array here
    arrayString staticResults;
    if (retResults == NULL) {
        retResults = &staticResults;
    }

    // First, discard anything that is pending
    mutexLock(&modemReceivedLinesMutex);
    for (int i=0; i<arrayEntries(&modemReceivedLines); i++) {
        debugf("modem: discarding \"%s\"\n", (char *) arrayEntry(&modemReceivedLines, i));
    }
    arrayReset(&modemReceivedLines);
    modemReceivedLineMs = 0;
    mutexUnlock(&modemReceivedLinesMutex);

    // Initialize results
    memset(retResults, 0, sizeof(arrayString));

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
            err = errF(ERR_TIMEOUT);
            break;
        }
        mutexLock(&modemReceivedLinesMutex);
        if (arrayEntries(&modemReceivedLines) == 0) {
            mutexUnlock(&modemReceivedLinesMutex);
            timerMsSleep(100);
            // In case we are being called from code in the modem task
            // itself, this is essential to process the input.
            processModemSerialIncoming();
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

    // Deallocate if static results
    if (retResults == &staticResults) {
        arrayReset(retResults);
    }

    // Done
    return err;

}

// Create an error if there aren't exactly N results
err_t modemRequireResults(arrayString *results, err_t err, uint32_t numResults, char *errType)
{
    if (err) {
        return errF("%s: %s", errType, errString(err));
    }
    if (numResults != arrayEntries(results)) {
        arrayReset(results);
        return errF("%s: %d results instead of %d", errType, arrayEntries(results), numResults);
    }
    return errNone;
}

// See if the modem has power
bool modemIsOn(void)
{
    return poweredOn;
}

// Turn on modem power
err_t modemPowerOn(void)
{
    err_t err;
    arrayString results;

    // Exit if already powered on
    if (poweredOn) {
        return errNone;
    }

    // Init the USART
    MX_USART2_UART_Init(false, 115200);

    // Send a reset command to the modem before powering it on.  This
    // will force a reset in case we are bypassing power control and
    // the modem is continuously powered.
    serialSendLineToModem("AT+QRST=1");

    // Physically power-on the modem
    poweredOn = true;

    // Wait until RDY, flushing everything else that comes in
    bool ready = false;
    int64_t timeoutMs = timerMs() + (30 * ms1Sec);
    while (!ready) {
        if (timerMs() >= timeoutMs) {
            modemPowerOff();
            return errF("modem power-on timeout");
        }
        processModemSerialIncoming();
        mutexLock(&modemReceivedLinesMutex);
        if (arrayEntries(&modemReceivedLines) == 0) {
            mutexUnlock(&modemReceivedLinesMutex);
            timerMsSleep(100);
            processModemSerialIncoming();
            continue;
        }
        for (int i=0; i<arrayEntries(&modemReceivedLines); i++) {
            char *line = arrayEntry(&modemReceivedLines, i);
            if (strEQL(line, "RDY")) {
                ready = true;
            }
        }
        arrayReset(&modemReceivedLines);
        modemReceivedLineMs = 0;
        mutexUnlock(&modemReceivedLinesMutex);
    }

    // Turn off echo
    err = errNone;
    for (int i=0; i<5; i++) {
        err = modemSend(NULL, "ATE0");
        if (!err) {
            break;
        }
    }
    if (err) {
        modemPowerOff();
        return err;
    }

    // Turn off 10 second auto sleep timer
    err = modemSend(NULL, "AT+QSCLK=0");
    if (err) {
        modemPowerOff();
        return err;
    }

    // Get IMSI if necessary
    if (modemId[0] == '\0') {
        err = modemSend(&results, "AT+CIMI");
        err = modemRequireResults(&results, err, 1, "can't get modem IMSI");
        if (err) {
            modemPowerOff();
            return err;
        }
        strLcpy(modemId, STARNOTE_SCHEME);
        strLcat(modemId, (char *) arrayEntry(&results, 0));
        arrayReset(&results);
    }

    // Get firmware version
    if (modemVersion[0] == '\0') {
        err = modemSend(&results, "AT+QGMR");
        err = modemRequireResults(&results, err, 1, "can't get modem firmware version");
        if (err) {
            modemPowerOff();
            return err;
        }
        strLcpy(modemVersion, (char *) arrayEntry(&results, 0));
        arrayReset(&results);
    }

    // Done
    return errNone;

}

// Turn off modem power
void modemPowerOff(void)
{

    // Exit if already powered off
    if (!poweredOn) {
        return;
    }

    // Uninit the USART
    MX_USART2_UART_DeInit();
    
    // Power off the modem
    poweredOn = false;

}
