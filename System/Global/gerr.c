// Copyright 2017 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

// This package implements the "error" semantic, which as implemented here is GC-based so that
// errors can be concatenated and handled in a golang-like manner and yet they can be ignored
// without deallocations.

#include <stdarg.h>
#include "global.h"
#include "board.h"
#include "mutex.h"

// The core error entry data structure
typedef struct {
    uint16_t errorTextOffset;
} errorEntry_t;

// These globals constrain the number of concurrent errors that may be being processed at any given
// moment in time before losing context.  There is no downside to increasing the number other than memory.
#define MAXCONCURRENTERRORS     15
#define MAXCONCURRENTERRORTEXT  (256*3)
STATIC errorEntry_t errors[MAXCONCURRENTERRORS];
STATIC uint16_t errorsNext = 0;
STATIC char errorText[MAXCONCURRENTERRORTEXT];
STATIC uint16_t errorTextNext = 0;
STATIC mutex errorMutex = {MTX_ERR, {0}};

// The fact that we offset by 1 simply guarantees that we never issue noError as a result
#define errorEntryToError(x) ((err_t)(x+1))
#define errorEntryFromError(x) (((uint16_t)x)-1)

// Print an error, cascading it by adding the previous error as a suffix.  If there's nothing to cascade,
// supply 0 as the first argument.  If format == NULL, we are guaranteed to return errNone;
err_t errF(const char *format, ...)
{
    char buffer[MAXERRSTRING];
    char *buf = buffer;
    int buflen = sizeof(buffer);
    buf[0] = '\0';

    // Exit if errF(NULL) is encountered - relied upon in file.c uses of FlashLock()
    if (format == NULL) {
        return errNone;
    }

    // Process the printf portion of the work to be done
    va_list args;
    va_start (args, format);
    vsnprintf (buf, buflen, format, args);
    va_end (args);

    // Protect statics
    mutexLock(&errorMutex);

    // Insert the cleaned text into the error buffer, because the error may have been generated
    // using user-generated data which has binary garbage within it
    buflen = strlen(buffer) + 1;
    if (buflen > (MAXCONCURRENTERRORTEXT - errorTextNext)) {
        errorTextNext = 0;
    }
    for (int i=0;; i++) {
        if (buffer[i] == 0 || i == (MAXCONCURRENTERRORTEXT-errorTextNext)-1) {
            errorText[errorTextNext+i] = '\0';
            break;
        }
        if (buffer[i] >= ' ' && buffer[i] < 0x7f) {
            errorText[errorTextNext+i] = buffer[i];
        } else {
            errorText[errorTextNext+i] = '?';
        }
    }
    errorEntry_t newEntry;
    newEntry.errorTextOffset = errorTextNext;

    // If this entry is identical to the previous entry, optimize it
    if (errorsNext > 0) {
        uint16_t errorsPrev = errorsNext-1;
        char *thisErrorText = &errorText[newEntry.errorTextOffset];
        uint32_t thisErrorTextLen = strlen(thisErrorText);
        char *prevErrorText = &errorText[errors[errorsPrev].errorTextOffset];
        uint32_t prevErrorTextLen = strlen(prevErrorText);
        if (thisErrorTextLen == prevErrorTextLen && memeql(thisErrorText, prevErrorText, thisErrorTextLen)) {
            mutexUnlock(&errorMutex);
            return errorEntryToError(errorsPrev);
        }
    }

    // Allocate the next error entry and fill it in
    errorTextNext += buflen;
    if (errorsNext >= MAXCONCURRENTERRORS) {
        errorsNext = 0;
    }
    err_t errorToReturn = errorEntryToError(errorsNext);
    errors[errorsNext++] = newEntry;

    // Release
    mutexUnlock(&errorMutex);

    // Return the new error entry
    return errorToReturn;

}

// Convert an error number to a string, using safe strlcat rules
char *errString(err_t err)
{

    // If no error, there's no text.
    if (!err) {
        return "";
    }

    // Look up the error entry and validate it, just to be defensive
    uint16_t entry = errorEntryFromError(err);
    if (entry >= MAXCONCURRENTERRORS) {
        return "invalid error code";
    }

    // Protect statics
    mutexLock(&errorMutex);

    // Get the error text.  Note that even if the errorText buffer wrapped, we will end up with safe
    // output here even if it will be contextually inaccurate.  That said, we code defensively.
    if (errors[entry].errorTextOffset >= MAXCONCURRENTERRORTEXT) {
        mutexUnlock(&errorMutex);
        return "error table corruption";
    }
    errorText[sizeof(errorText)-1] = '\0';

    // All valid error strings either begin at the start of the buffer, or are immediately following
    // a prior null-terminated error string.  This allows us to detect circular overlap.
    if (errors[entry].errorTextOffset > 0)
        if (errorText[errors[entry].errorTextOffset-1] != '\0') {
            mutexUnlock(&errorMutex);
            return "an unknown error occurred (high error rate)";
        }

    // Release
    mutexUnlock(&errorMutex);

    // Done
    return (&errorText[errors[entry].errorTextOffset]);

}

// Convert error to a body structure
err_t errBody(err_t err, uint8_t **retBody, uint32_t *retBodyLen)
{
    char *errPrefix = "{\"err\":\"";
    uint32_t prefixLen = strlen(errPrefix);
    char *errSuffix = "\"}";
    uint32_t suffixLen = strlen(errSuffix);
    char *errstr = errString(err);
    uint32_t errLen = strlen(errstr);
    uint32_t bodylen = prefixLen + errLen + suffixLen;
    uint8_t *body;
    err_t err2 = memAlloc(bodylen, &body);
    if (err2) {
        return err2;
    }
    memcpy(body, errPrefix, prefixLen);
    for (int i=0; i<errLen; i++) {
        if (errstr[i] == '"') {
            body[prefixLen+i] = '\'';
        } else {
            body[prefixLen+i] = errstr[i];
        }
    }
    memcpy(&body[prefixLen+errLen], errSuffix, suffixLen);
    *retBody = body;
    *retBodyLen = bodylen;
    return errNone;
}

// Test to see if an error contains an error keyword that we might expect
bool errContains(err_t err, const char *errKeyword)
{

    // Exit if no error
    if (!err) {
        return false;
    }

    // Look for the presence of the precise substring in its entirety
    return strstr(errString(err), errKeyword) != NULL;

}
