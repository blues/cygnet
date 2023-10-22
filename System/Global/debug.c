// Copyright 2021 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include <stdarg.h>
#include "global.h"
#include "main.h"

// Output a debug string raw
void debugR(const char *strFormat, ...)
{
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
    MX_DBG(buf, strlen(buf));
}

// Output a simple string with length
void debugMessageLen(const char *buf, uint32_t buflen)
{
    MX_DBG(buf, buflen);
}

// Output a debug string with timer
void debugf(const char *strFormat, ...)
{
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
    MX_USART1_Message("*****\n");
    MX_USART1_Message((char *)message);
    MX_USART1_Message("\n*****\n");
    MX_Breakpoint();
}

// Panic
void debugPanic(const char *message)
{
    debugSoftPanic(message);
    NVIC_SystemReset();
}

// Return a pointer to the filename portion of a path.  Do NOT use mutexes, because this is
// too low level and is called by the mutex code itself.
char *debugFileName(const char *fileName)
{
    if (fileName == NULL) {
        return "";
    }
    char *lastName = (char *) fileName;
    for (;;) {
        char ch = *fileName++;
        if (ch == '\0') {
            break;
        }
        if (ch == '/' || ch == '\\') {
            lastName = (char *) fileName;
        }
    }
    return lastName;
}
