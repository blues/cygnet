// Copyright 2017 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "app.h"
#include "main.h"
#include "global.h"
#include "mutex.h"
#include "rtc.h"
#include <time.h>

// Boot time
STATIC int64_t bootTimeMs = 0;

// Protect clib
STATIC mutex timeMutex = {MTX_TIME, {0}};

// timegm.c
extern time_t rk_timegm (struct tm *tm);

// Get the approximate number of seconds since boot
int64_t timerMsSinceBoot()
{
    int64_t nowMs = timerMs();
    return (nowMs - bootTimeMs);
}

// Get the time when boot happened
uint32_t timeSecsBoot()
{
    if (!timeIsValid()) {
        return 0;
    }
    return timeSecs() - (timerMsSinceBoot() / ms1Sec);
}

// Reset the boot timer, for debugging
void timerSetBootTime()
{
    bootTimeMs = timerMs();
    // Make sure that we're compiling in a way that prevents the Epochalypse
    if (sizeof(time_t) != sizeof(uint64_t)) {
        debugPanic("epochalypse");
    }
}

// Get the current timer tick in milliseconds, for elapsed time computation purposes.
int64_t timerMs(void)
{
    return MX_GetTickMs();
}

// Get the current timer tick in milliseconds from within an ISR, knowing that it will NOT
// be changing from the value returned (and thus there must be no timing loops)
int64_t timerMsFromISR(void)
{
    return MX_GetTickMsFromISR();
}

// Delay for the specified number of milliseconds in a compute loop, without yielding.
// This is primarily used for operations that could potentially
// be executed in STOP2 mode, during which our task timers are not accurate.
void timerMsDelay(uint32_t ms)
{
    HAL_Delay(ms);
}

// Sleep for the specified number of milliseconds or until a signal occurs, whichever comes first,
// processing timers as necessary.
void timerMsSleep(uint32_t ms)
{
    vTaskDelay(ms);
}

// See if the specified number of milliseconds has elapsed
bool timerMsElapsed(int64_t began, uint32_t ms)
{
    return (began == 0 || timerMs() >= (began + ms));
}

// Compute the interval between now and the specified "future" timer, returning 0 if it is in the past
uint32_t timerMsUntil(int64_t suppressionTimerMs)
{
    int64_t now = timerMs();
    if (suppressionTimerMs < now) {
        return 0;
    }
    return ((uint32_t)(suppressionTimerMs - now));
}

// Set the current time/date
bool timeSet(uint32_t newTimeSecs)
{

    // Ensure that we notice when the time was set
    timeIsValid();

    // Ensure that it's valid
    if (!timeIsValidUnix(newTimeSecs)) {
        return false;
    }

    // Lock
    mutexLock(&timeMutex);
    time_t timenow = (time_t) newTimeSecs;

    // Do the conversion and unlock
#ifdef USING_IAR    // Not thread-safe.  Should use gmtime_s but compiling with __STDC_WANT_LIB_EXT1__ requires huge labor.
    struct tm *t = gmtime(&timenow);
#else               // POSIX
    struct tm tmp;
    struct tm *t = gmtime_r(&timenow, &tmp);
#endif
    if (t == NULL) {
        mutexUnlock(&timeMutex);    // cannot do debugf with locked
        debugf("time-set: error getting struct tm\n");
        return false;
    }
    int year = t->tm_year+1900;
    int mon1 = t->tm_mon+1;
    int day1 = t->tm_mday;
    int hour0 = t->tm_hour;
    int min0 = t->tm_min;
    int sec0 = t->tm_sec;
    if (!MX_RTC_SetDateTime(year, mon1, day1, hour0, min0, sec0)) {
        mutexUnlock(&timeMutex);
        debugf("time-set: HAL error setting date/time\n");
        return false;
    }
    mutexUnlock(&timeMutex);
    debugf("time-set: %d %04d-%02d-%02dT%02d:%02d:%02dZ\n", newTimeSecs, year, mon1, day1, hour0, min0, sec0);
    return true;
}

// See if the time is currently valid, and try to restore it from the RTC if not
bool timeIsValid()
{
    if (!timeIsValidUnix(timeSecs())) {
        return false;
    }
    return true;
}

// See if the time is plausibly a valid Unix time for the operations of our module,
// which is determined by the build date.
// Function taking a uint32_t argument in seconds.
bool timeIsValidUnix(uint32_t t)
{
    return (t > 1684171787 && t < 0xfffffff0);
}

// Get the current time in seconds, rather than Ns.  This has a special semantic in that it is intended
// to be used for time interval comparisons.  As such, if the time isn't yet initialized, it returns
// the number of seconds since boot rather than the actual calendar time, which works as a substitute.
// When the time gets set ultimately, it will be a major leap forward.
uint32_t timeSecs(void)
{
    return (uint32_t)(MX_RTC_GetMs() / 1000LL);
}

// See if the specified number of seconds has elapsed
bool timeSecsElapsed(int64_t began, uint32_t secs)
{
    return (timeSecs() >= (began + secs));
}

// Convert decimal to two digits
static char *twoDigits(int n, char *p)
{
    *p++ = '0' + ((n / 10) % 10);
    *p++ = '0' + n % 10;
    return p;
}

// Convert decimal to four digits
static char *fourDigits(int n, char *p)
{
    *p++ = '0' + ((n / 1000) % 10);
    *p++ = '0' + ((n / 100) % 10);
    *p++ = '0' + ((n / 10) % 10);
    *p++ = '0' + n % 10;
    return p;
}

// Get the current time as a string using as little stack space as possible because used deep in log processing
void timeStr(uint32_t time, char *str, uint32_t length)
{
    if (length < 10) {
        strlcpy(str, "(too small)", length);
        return;
    }
    if (time == 0) {
        strlcpy(str, "(uninitialized)", length);
        return;
    }
    mutexLock(&timeMutex);
    time_t t1 = (time_t) time;
    // Not thread-safe.  Should use gmtime_s but compiling with __STDC_WANT_LIB_EXT1__ requires huge labor.
    struct tm *t2 = gmtime(&t1);
    str = twoDigits(t2->tm_hour, str);
    *str++ = ':';
    str = twoDigits(t2->tm_min, str);
    *str++ = ':';
    str = twoDigits(t2->tm_sec, str);
    *str++ = 'Z';
    *str = '\0';
    mutexUnlock(&timeMutex);
    return;
}

// Get the time components from the specified time.  Note that offsets should be self-evident by variable names
bool timeLocal(uint32_t time, uint32_t *year, uint32_t *mon1, uint32_t *day1, uint32_t *hour0, uint32_t *min0, uint32_t *sec0, uint32_t *wdaySun0)
{
    if (time == 0) {
        return false;
    }
    mutexLock(&timeMutex);
    time_t timenow = (time_t) time;
    // Not thread-safe.  Should use gmtime_s but compiling with __STDC_WANT_LIB_EXT1__ requires huge labor.
    struct tm *t = gmtime(&timenow);
    if (t == NULL || t->tm_year == 0) {
        mutexUnlock(&timeMutex);    // cannot do debugf with locked
        return false;
    }
    *year = (uint32_t) t->tm_year+1900;
    *mon1 = (uint32_t) t->tm_mon+1;
    *day1 = (uint32_t) t->tm_mday;
    *hour0 = (uint32_t) t->tm_hour;
    *min0 = (uint32_t) t->tm_min;
    *sec0 = (uint32_t) t->tm_sec;
    *wdaySun0 = (uint32_t) t->tm_wday;
    mutexUnlock(&timeMutex);
    return true;
}

// Get the current time as a string using as little stack space as possible because used deep in log processing
void timeDateStr(uint32_t time, char *str, uint32_t length)
{
    if (length < 21) {
        strlcpy(str, "(too small)", length);
        return;
    }
    if (time == 0) {
        strlcpy(str, "(uninitialized)", length);
        return;
    }
    mutexLock(&timeMutex);
    time_t t1 = (time_t) time;
    // Not thread-safe.  Should use gmtime_s but compiling with __STDC_WANT_LIB_EXT1__ requires huge labor.
    struct tm *t2 = gmtime(&t1);
    str = fourDigits(t2->tm_year+1900, str);
    *str++ = '-';
    str = twoDigits(t2->tm_mon+1, str);
    *str++ = '-';
    str = twoDigits(t2->tm_mday, str);
    *str++ = 'T';
    str = twoDigits(t2->tm_hour, str);
    *str++ = ':';
    str = twoDigits(t2->tm_min, str);
    *str++ = ':';
    str = twoDigits(t2->tm_sec, str);
    *str++ = 'Z';
    *str = '\0';
    mutexUnlock(&timeMutex);
    return;
}

// Get HH:MM:SS for debugging, from seconds
void timeSecondsToHMS(uint32_t sec, char *buf, uint32_t buflen)
{
    int secs = (int) sec;
    int mins = secs/60;
    secs -= mins*60;
    int hrs = mins/60;
    mins -= hrs*60;
    if (hrs > 0) {
        snprintf(buf, buflen, "%dh %dm %ds", hrs, mins, secs);
    } else if (mins > 0) {
        snprintf(buf, buflen, "%dm %ds", mins, secs);
    } else {
        snprintf(buf, buflen, "%ds", secs);
    }
}
