// Copyright 2021 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include <stdarg.h>
#include "global.h"
#include "main.h"
#include <stdatomic.h>

STATIC atomic_int debugPaused = 0;

// Suppress debug output temporarily
void debugPause(void)
{
    atomic_fetch_add(&debugPaused, 1);
}

// See if debug is paused
bool debugIsPaused(void)
{
    return atomic_load(&debugPaused) != 0;
}

// Resume debug output after pausing temporarily
void debugResume(void)
{
    atomic_fetch_sub(&debugPaused, 1);
}

// Output a debug string raw
void debugR(const char *strFormat, ...)
{
    if (atomic_load(&debugPaused) != 0) {
        return;
    }
    char buf[MAXERRSTRING];
    va_list vaArgs;
    va_start(vaArgs, strFormat);
    vsnprintf(buf, sizeof(buf), strFormat, vaArgs);
    MX_DBG(buf, strlen(buf));
    va_end(vaArgs);
}

// Output a simple string
void debugMessage(const char *buf)
{
    if (atomic_load(&debugPaused) != 0) {
        return;
    }
    MX_DBG(buf, strlen(buf));
}

// Output a simple string with length
void debugMessageLen(const char *buf, uint32_t buflen)
{
    if (atomic_load(&debugPaused) != 0) {
        return;
    }
    MX_DBG(buf, buflen);
}

// Output a debug string with timer
void debugf(const char *strFormat, ...)
{
    if (atomic_load(&debugPaused) != 0) {
        return;
    }
    char buf[MAXERRSTRING];
    va_list vaArgs;
    va_start(vaArgs, strFormat);
    vsnprintf(buf, sizeof(buf), strFormat, vaArgs);
    MX_DBG(buf, strlen(buf));
    va_end(vaArgs);
}

// Breakpoint
void debugBreakpoint(void)
{
    MX_Breakpoint();
}

// Soft panic
void debugSoftPanic(const char *message)
{
    debugMessage("*****\n");
    debugMessage((char *)message);
    debugMessage("\n*****\n");
    MX_Breakpoint();
}

// Panic
void debugPanic(const char *message)
{
    debugSoftPanic(message);
    NVIC_SystemReset();
}
