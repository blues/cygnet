
#include <stdarg.h>
#include "app.h"
#include "serial.h"
#include "usart.h"

// Modem work to do
typedef struct {
    const char *status;
    modemWorker worker;
    J *workBody;
    uint8_t *workPayload;
    uint32_t workPayloadLen;
} work_t;
STATIC mutex modemWorkMutex = {MTX_MODEM_WORK, {0}};
STATIC array *work = NULL;
STATIC const char *workStatus = NULL;

// Received lines.  Note that we separate normal lines from
// URC lines, and we always ensure uniqueness of URCs.
STATIC mutex modemReceivedLinesMutex = {MTX_MODEM_RECEIVED, {0}};
STATIC arrayString modemReceivedLines = {0};
STATIC arrayString modemReceivedUrcLines = {0};

// Identity
char modemId[48] = {0};
char modemVersion[48] = {0};

// Modem state
STATIC bool poweredOn = false;
STATIC bool isReady = false;
STATIC bool connected = false;

// Forwards
bool processModemWork(void);
bool processUrcWork(void);
bool processSerialIncoming(void);
void uModemUrcRemove(char *urc);

// Request task
void modemTask(void *params)
{

    // Init task
    taskRegister(TASKID_MODEM, TASKNAME_MODEM, TASKLETTER_MODEM, TASKSTACK_MODEM);

    // Loop, extracting requests from the serial port and processing them
    while (true) {
        bool didSomething = false;
        didSomething |= processSerialIncoming();
        if (!didSomething) {
            didSomething |= processUrcWork();
        }
        if (!didSomething) {
            didSomething |= processModemWork();
        }
        if (!didSomething) {
            taskTake(TASKID_MODEM, modemIsOn() ? ms1Sec : ms1Day);
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

// Ensure that the work queue is allocated
err_t modemInit(void)
{

    // Allocate the work queue
    err_t err = arrayAlloc(sizeof(work_t), modemWorkReset, &work);
    if (err) {
        return err;
    }

    return errNone;
}

// Enqueue work.  Note that in all cases (even errors) workToDo is freed if supplied,
// and b64Payload is NOT freed.
err_t modemEnqueueWork(const char *status, modemWorker worker, J *workBody, char *workPayloadB64)
{
    err_t err;

    // Append the work
    work_t w = {0};
    w.status = status;
    w.worker = worker;
    w.workBody = (workBody == NULL ? JCreateObject() : workBody);

    // Allocate the payload if it's supplied.  As a convenience, guarantee
    // that the payload buffer is one larger than needed and null-terminated.
    if (workPayloadB64 != NULL && *workPayloadB64 != '\0') {
        err = memAlloc(JB64DecodeLen(workPayloadB64)+1, &w.workPayload);
        if (err) {
            JDelete(w.workBody);
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

    taskGive(TASKID_MODEM);

    return errNone;

}

// Remove a URC from the received URCs, unlocked
void uModemUrcRemove(char *urc)
{
    if (urc == NULL) {
        arrayReset(&modemReceivedUrcLines);
    } else {
        char *colon = strchr(urc, ':');
        uint32_t prefixLen = (colon == NULL ? strlen(urc) : (colon-urc)) + 1;
        for (int i=0; i<arrayEntries(&modemReceivedUrcLines); i++) {
            if (memeql(urc, arrayEntry(&modemReceivedUrcLines, i), prefixLen)) {
                arrayRemove(&modemReceivedUrcLines, i);
            }
        }
    }
}

// Remove a URC from the received URCs
void modemUrcRemove(char *urc)
{
    mutexLock(&modemReceivedLinesMutex);
    uModemUrcRemove(urc);
    mutexUnlock(&modemReceivedLinesMutex);
}

// Get a URC from the received URCs
bool modemUrcGet(char *urc, char *buf, uint32_t buflen)
{
    mutexLock(&modemReceivedLinesMutex);
    char *colon = strchr(urc, ':');
    uint32_t prefixLen = (colon == NULL ? strlen(urc) : (colon-urc)) + 1;
    for (int i=0; i<arrayEntries(&modemReceivedUrcLines); i++) {
        char *line = arrayEntry(&modemReceivedUrcLines, i);
        if (memeql(urc, line, prefixLen)) {
            strlcpy(buf, line, buflen);
            mutexUnlock(&modemReceivedLinesMutex);
            return true;
        }
    }
    mutexUnlock(&modemReceivedLinesMutex);
    return false;
}

// Show URC state
void modemUrcShow()
{
    if (!modemIsOn()) {
        debugR("modem is off\n");
    } else {
        mutexLock(&modemReceivedLinesMutex);
        for (int i=0; i<arrayEntries(&modemReceivedUrcLines); i++) {
            debugR("%s\n", (char *) arrayEntry(&modemReceivedUrcLines, i));
        }
        mutexUnlock(&modemReceivedLinesMutex);
    }
}

// Test to see if a URC is present with an exact value
bool modemUrc(char *urc, bool remove)
{
    mutexLock(&modemReceivedLinesMutex);
    for (int i=0; i<arrayEntries(&modemReceivedUrcLines); i++) {
        if (streql(urc, arrayEntry(&modemReceivedUrcLines, i))) {
            if (remove) {
                arrayRemove(&modemReceivedUrcLines, i);
            }
            mutexUnlock(&modemReceivedLinesMutex);
            return true;
        }
    }
    mutexUnlock(&modemReceivedLinesMutex);
    return false;
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

// Process one unit of pending URC work
bool processUrcWork(void)
{

    // Exit if modem is not powered on
    if (!modemIsOn() || !isReady) {
        return false;
    }

    // Send a location update if requested by the modem
    if (modemUrc("+QGNSSINFO: 1", true)) {
        double lat, lon;
        if (locGet(&lat, &lon, NULL)) {
            modemSend(NULL, "AT+QGNSSINFO=%f,%f,0,0,0", lat, lon);
        }
        return true;
    }

    // Nothing done
    return false;

}

// Process one unit of pending modem work
bool processModemWork(void)
{

    // Don't do anything while modem info is still needed
    if (modemInfoNeeded()) {
        return false;
    }

    // Process work
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
        workStatus = w.status;
        err_t err = w.worker(w.workBody, w.workPayload, w.workPayloadLen);
        workStatus = NULL;
        if (w.worker == workModemConnect || w.worker == workModemDisconnect) {
            modemReportStatus();
        }
        modemWorkReset(&w);
        if (err) {
            debugf("modem: %s\n", errString(err));
        }
    }

    return workToDo;

}

// Process incoming serial request and URCs at a time when
// the modem AT command state should be idle
void modemProcessSerialIncoming(void)
{
    processSerialIncoming();
    processUrcWork();
}

// Process incoming serial request
bool processSerialIncoming(void)
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

    // Perform special handling of the downlink URC
    if (memeql(modemResponse, "+CRTDCP: ", 9)) {
        char *q1 = strchr((char *)modemResponse, '"');
        if (q1 != NULL) {
            char *hex = q1+1;
            char *q2 = strchr(hex, '"');
            if (q2 != NULL) {
                *q2 = '\0';
                J *work = JCreateString(hex);
                modemEnqueueWork(STATUS_WORK_DOWNLINKING, workModemDownlink, work, NULL);
            }
        }
        modemResponseLen = 0;
    }

    // Add the line to an array of received lines.  Note that this depends upon
    // the fact that serial.c pollPort() uses arrayAppendStringBytes in such a
    // way that the buffer here is guaranteed to be null-terminated.
    if (modemResponseLen > 0) {
        mutexLock(&modemReceivedLinesMutex);
        if (modemResponse[0] == '+') {
            uModemUrcRemove((char *)modemResponse);
            arrayAppend(&modemReceivedUrcLines, modemResponse);
        } else {
            arrayAppend(&modemReceivedLines, modemResponse);
        }
        mutexUnlock(&modemReceivedLinesMutex);
    }

    // Copy into trace buffer while the pointer is still valid.
    char trace[64];
    strLcpy(trace, (char *)modemResponse);

    // Unlock serial
    serialUnlock(huart, true);

    // Trace, but never do this with serial locked
    debugR("modem: << %s\n", modemResponse);

    // Done
    return true;

}

// Send a modem command and get its response(s)
err_t modemSend(arrayString *retResults, char *format, ...)
{
    err_t err;

    // Exit if already powered off
#ifndef MODEM_ALWAYS_ON
    if (!poweredOn) {
        return errF("ntn: modem not powered on");
    }
#endif

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
    int64_t expiresMs = timerMs() + 5000;
    bool terminatorReceived = false;
    err = errNone;
    while (!err && !terminatorReceived) {
        if (timerMs() > expiresMs) {
            err = errF(ERR_TIMEOUT);
            break;
        }
        mutexLock(&modemReceivedLinesMutex);
        if (arrayEntries(&modemReceivedLines) == 0) {
            mutexUnlock(&modemReceivedLinesMutex);
            timerMsSleep(1);
            // In case we are being called from code in the modem task
            // itself, this is essential to process the input.
            processSerialIncoming();
            continue;
        }
        expiresMs = timerMs() + 1000;
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

    // Process URC work that may have occurred in the middle of AT command processing
    processUrcWork();

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

// Return the index of a result containing the specified prefix, or -1 if not there
int modemResult(arrayString *results, char *prefix)
{
    uint32_t prefixLen = strlen(prefix);
    for (int i=0; i<arrayEntries(results); i++) {
        if (memeql(prefix, (char *) arrayEntry(results, i), prefixLen)) {
            return i;
        }
    }
    return -1;
}

// See if the modem has power
bool modemIsOn(void)
{
    return poweredOn;
}

// See if the modem has power
bool modemIsConnected(void)
{
    return connected;
}

// Set whether or not we're connected
void modemSetConnected(bool yesOrNo)
{
    connected = yesOrNo;
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

    // Cleaer all pending URCs
    modemUrcRemove(NULL);

    // Init the USART
#ifndef MODEM_ALWAYS_ON
    MX_USART2_UART_Init(false, 115200);
#else
    serialSendLineToModem("AT+QRST=1");
#endif

    // Physically power-on the modem
    isReady = false;
    poweredOn = true;
    debugf("modem: powered on\n");

    // Wait until RDY, flushing everything else that comes in
    bool ready = false;
    int64_t timeoutMs = timerMs() + (30 * ms1Sec);
    while (!ready) {
        if (timerMs() >= timeoutMs) {
            modemPowerOff();
            return errF("modem power-on timeout");
        }
        processSerialIncoming();
        mutexLock(&modemReceivedLinesMutex);
        if (arrayEntries(&modemReceivedLines) == 0) {
            mutexUnlock(&modemReceivedLinesMutex);
            timerMsSleep(100);
            processSerialIncoming();
            continue;
        }
        for (int i=0; i<arrayEntries(&modemReceivedLines); i++) {
            char *line = arrayEntry(&modemReceivedLines, i);
            if (strEQL(line, "RDY")) {
                ready = true;
            }
        }
        arrayReset(&modemReceivedLines);
        mutexUnlock(&modemReceivedLinesMutex);
    }

    // Turn off echo
    err = errNone;
    for (int i=0; i<5; i++) {
        timerMsSleep(3000);
        err = modemSend(NULL, "ATE0");
        if (!err) {
            break;
        }
    }
    if (err) {
        modemPowerOff();
        return err;
    }

    // Wait for stability
    timerMsSleep(2000);
    isReady = true;

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
#ifndef MODEM_ALWAYS_ON
    MX_USART2_UART_DeInit();
#endif

    // Power off the modem
    poweredOn = false;
    isReady = false;
    modemSetConnected(false);
    debugf("modem: powered off\n");

}

// Report the current status
err_t modemReportStatus(void)
{

    array *statusBytes;
    err_t err = arrayAllocBytes(&statusBytes);
    if (err) {
        return err;
    }

    if (modemIsOn()) {
        arrayAppendStringBytes(statusBytes, STATUS_POWER);
    }
    if (modemIsConnected()) {
        arrayAppendStringBytes(statusBytes, STATUS_CONNECTED);
    }
    volatile const char *status = workStatus;
    if (status != NULL) {
        arrayAppendStringBytes(statusBytes, (char *) status);
    } else if (modemInfoNeeded()) {
        arrayAppendStringBytes(statusBytes, STATUS_WORK_INIT);
    }

    char *p = (char *) arrayAddress(statusBytes);
    if (p == NULL || p[0] == '\0') {
        p = STATUS_IDLE;
    }
    serialSendMessageToNotecard(serialCreateMessage(ReqStatus, p, NULL, NULL, 0));
    arrayFree(statusBytes);

    return errNone;

}

// See if modem info is still needed
bool modemInfoNeeded(void)
{
    return (modemId[0] == '\0' || modemVersion[0] == '\0');
}
