
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
char workDetailedStatus[64] = {0};

// Received lines.  Note that we separate normal lines from
// URC lines, and we always ensure uniqueness of URCs.
STATIC mutex modemReceivedLinesMutex = {MTX_MODEM_RECEIVED, {0}};
STATIC arrayString modemReceivedLines = {0};
STATIC arrayString modemReceivedUrcLines = {0};

// Modem state
STATIC bool connected = false;
STATIC bool modemConnectFailure = false;

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
            taskTake(TASKID_MODEM, modemPoweredOn ? ms1Sec : ms1Day);
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
    if (!modemPoweredOn) {
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

// See if work already exists
bool modemWorkExists(modemWorker worker)
{
    bool found = false;
    mutexLock(&modemWorkMutex);
    for (int i=0; i<arrayEntries(work); i++) {
        work_t *w = (work_t *) arrayEntry(work, i);
        if (w->worker == worker) {
            found = true;
            break;
        }
    }
    mutexUnlock(&modemWorkMutex);
    return found;
}

// Process one unit of pending URC work
bool processUrcWork(void)
{

    // Exit if modem is not powered on
    if (!modemPoweredOn || !modemIsReady) {
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
        workDetailedStatus[0] = '\0';
        workStatus = w.status;
        if (w.worker == workModemConnect || w.worker == workModemDisconnect) {
            if (w.worker == workModemConnect) {
                modemConnectFailure = false;
            }
            modemReportStatus();
        }
        err_t err = w.worker(w.workBody, w.workPayload, w.workPayloadLen);
        workStatus = NULL;
        if (w.worker == workModemConnect || w.worker == workModemDisconnect) {
            if (w.worker == workModemConnect) {
                modemConnectFailure = (err != errNone);
            }
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

// See if the modem has received the specified line
bool modemHasReceivedLine(char *line)
{
    timerMsSleep(100);
    processSerialIncoming();
    mutexLock(&modemReceivedLinesMutex);
    if (arrayEntries(&modemReceivedLines) != 0) {
        for (int i=0; i<arrayEntries(&modemReceivedLines); i++) {
            char *line = arrayEntry(&modemReceivedLines, i);
            if (strEQL(line, "RDY")) {
                mutexUnlock(&modemReceivedLinesMutex);
                return true;
            }
        }
    }
    mutexUnlock(&modemReceivedLinesMutex);
    return false;
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
                modemEnqueueWork(NTN_DOWNLINKING, workModemDownlink, work, NULL);
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
    if (!modemPoweredOn) {
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
    int64_t expiresMs = timerMs() + 10000;
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
bool modemIsConnected(void)
{
    return connected;
}

// Set whether or not we're connected
void modemSetConnected(bool yesOrNo)
{
    connected = yesOrNo;
}

// Report the current status
err_t modemReportStatus(void)
{

    array *statusBytes;
    err_t err = arrayAllocBytes(&statusBytes);
    if (err) {
        return err;
    }

    // See if it's currently performing work
    volatile const char *status = workStatus;
    if (status != NULL) {
        char msg[128] = {0};
        if (workDetailedStatus[0] == '\0') {
            strLcpy(msg, workStatus);
        } else {
            strLcpy(msg, workDetailedStatus);
            strLcat(msg, " ");
            strLcat(msg, workStatus);
        }
        arrayAppendStringBytes(statusBytes, msg);
    } else if (modemInfoNeeded()) {
        arrayAppendStringBytes(statusBytes, NTN_INIT);
    } else {
        arrayAppendStringBytes(statusBytes, NTN_IDLE);
    }

    // Append state
    if (modemPoweredOn) {
        arrayAppendStringBytes(statusBytes, NTN_POWER);
    }
    if (gpsPoweredOn) {
        arrayAppendStringBytes(statusBytes, NTN_GPS_ENABLED);
    }
    if (modemIsConnected()) {
        arrayAppendStringBytes(statusBytes, NTN_CONNECTED);
    }
    if (modemConnectFailure) {
        arrayAppendStringBytes(statusBytes, NTN_CONNECT_FAILURE);
    }
    if (!locGet(NULL, NULL, NULL)) {
        arrayAppendStringBytes(statusBytes, NTN_NO_LOCATION);
    }

    serialSendMessageToNotecard(serialCreateMessage(ReqStatus, (char *) arrayAddress(statusBytes), NULL, NULL, 0));
    arrayFree(statusBytes);

    return errNone;

}

// See if modem info is still needed
bool modemInfoNeeded(void)
{

    // See if they're cached
    if (configModemId[0] != '\0' && configModemVersion[0] != '\0') {
        return false;
    }

    // Try extracting them out of the test cert
    J *tcert = postGetCachedTestCert();
    if (tcert == NULL) {
        return true;
    }
    strLcpy(configModemId, JGetString(tcert, tcFieldDeviceUID));
    strLcpy(configModemVersion, JGetString(tcert, tcFieldModem));
    
    // Return whether or not they're still needed
    return (configModemId[0] == '\0' || configModemVersion[0] == '\0');

}
