// Copyright 2017 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include "amd5.h"

#pragma once

// Useful macros
#define UNUSED_VARIABLE(X)  ((void)(X))
#ifndef UNUSED_PARAMETER
#define UNUSED_PARAMETER(X) UNUSED_VARIABLE(X)
#endif

#define STATIC

#define STRINGIZE(x) #x
#define STRINGIZE_VALUE_OF(x) STRINGIZE(x)

#define GMAX(x, y) (((x) > (y)) ? (x) : (y))
#define GMIN(x, y) (((x) < (y)) ? (x) : (y))

// Golang-like fallthrough
#if !defined(__fallthrough) && __cplusplus > 201703L && defined(__has_cpp_attribute)
#if __has_cpp_attribute(fallthrough)
#define __fallthrough [[fallthrough]]
#elif __has_cpp_attribute(gnu::fallthrough)
#define __fallthrough [[gnu::fallthrough]]
#endif
#endif
#if !defined(__fallthrough) && defined(__has_attribute)
#if __has_attribute(__fallthrough__)
#define __fallthrough __attribute__((__fallthrough__))
#endif
#endif
#if !defined(__fallthrough) && defined(__GNUC__)
#define __fallthrough __attribute__((fallthrough))
#endif
#if !defined(__fallthrough)
#define __fallthrough ((void)0)
#endif

// Time helpers
#define secs1Min    (60)
#define mins1Hour   (60)
#define hours1Day   (24)
#define mins1Day    (mins1Hour*hours1Day)
#define days1Week   (7)
#define mins1Week   (mins1Day*days1Week)
#define ms1Sec      (1000)
#define us1ms       (1000)
#define us1Sec      (1000000L)
#define ns1Sec      (1000000000L)
#define ms1Min      (secs1Min*ms1Sec)
#define secs1Hour   (secs1Min*mins1Hour)
#define ms1Hour     (mins1Hour*ms1Min)
#define ms1Day      (hours1Day*ms1Hour)
#define ms1Week     (ms1Day*days1Week)
#define secs1Day    (secs1Hour*hours1Day)
#define secs1Week   (secs1Hour*hours1Day*days1Week)
#define secs30Days  (30*secs1Day)
#define secs365Days (secs1Day*365)

// String helpers
#define streql(a,b) (0 == strcmp(a,b))
#define strEQL(a,b) (((uint8_t *)(a))[sizeof(b "")-1] != '\0' ? false : 0 == memcmp(a,b,sizeof(b)))
#define strNULL(x) (((x) == NULL) || ((x)[0] == '\0'))
#define memeql(a,b,c) (0 == memcmp(a,b,c))
#define memeqlstr(a,b) (0 == memcmp(a,b,strlen(b)))

// Make sure that we ONLY use strl's, not str's or strn's
#ifndef NO_STR_WARNINGS
#ifdef strcpy
#undef strcpy
#endif
#ifdef strcat
#undef strcat
#endif
#ifdef strncpy
#undef strncpy
#endif
#ifdef strncat
#undef strncat
#endif
#ifdef sprintf
#undef sprintf
#endif
#define strcpy { #error "NO STRCPY - USE STRLCPY" }
#define strcat { #error "NO STRCAT - USE STRLCAT" }
#define strncpy { #error "NO STRNCPY - USE STRLCPY" }
#define strncat { #error "NO STRNCAT - USE STRLCAT" }
#define sprintf { #error "NO SPRINTF - USE SNPRINTF" }
#endif
#ifndef LIBBSD
#ifndef strlcpy
size_t strlcpy(char *dst, const char *src, size_t siz);
#endif
#ifndef strlcat
size_t strlcat(char *dst, const char *src, size_t siz);
#endif
#endif
#define strLcpy(a,b)                                                    \
    ({_Static_assert(sizeof((a)) != 4, "Warning:strLcpy with size 4");  \
        strlcpy(a,b,sizeof((a))); })
#define strLcat(a,b)                                                    \
    ({_Static_assert(sizeof((a)) != 4, "Warning:strLcat with size 4");  \
        strlcat(a,b,sizeof((a))); })

// Misc string stuff
#define isAsciiAlpha(x) (((x) >= 'a' && (x) <= 'z') || ((x) >= 'A' && (x) <= 'Z'))
#define isAsciiNumeric(x) ((x) >= '0' && (x) <= '9')
#define isAsciiNumericSigned(x) (((x) >= '0' && (x) <= '9') || ((x) == '-'))
#define isAsciiAlphaNumeric(x) (isAsciiAlpha(x) || isAsciiNumeric(x))
#define isAsciiAlphaNumericSigned(x) (isAsciiAlpha(x) || isAsciiNumericSigned(x))

// gerr.c
typedef int32_t err_t;
#define MAXERRSTRING 256
#define errNone ((err_t)0)
err_t errF(const char *format, ...);
err_t errBody(err_t err, uint8_t **retBody, uint32_t *retBodyLen);
bool errContains(err_t err, const char *errkey);
char *errString(err_t err);

// debug.c
void debugBreakpoint(void);
void debugf(const char *format, ...);
void debugR(const char *format, ...);
void debugMessage(const char *buf);
void debugMessageLen(const char *buf, uint32_t buflen);
char *debugFileName(const char *fileName);
void debugPanic(const char *message);
void debugSoftPanic(const char *message);

// gmem.c
extern long memObjects;
extern long memFailures;
#define memAllocatedObjects() memObjects
#define memAllocationFailures() memFailures
uint32_t memCurrentlyFree(void);
err_t memAlloc(uint32_t length, void *ptr);
void memFree(void *p);
err_t memRealloc(uint32_t fromLength, uint32_t toLength, void *ptr);
err_t memDup(void *pSrc, uint32_t srcLength, void *pCopy);

// loc.c
bool locSet(double lat, double lon);
bool locGet(double *lat, double *lon, uint32_t *when);
bool locValid(void);
void locInvalidate(void);

// timer.c
int64_t timerMsSinceBoot(void);
uint32_t timeSecsBoot(void);
void timerSetBootTime(void);
int64_t timerMs(void);
void timerMsDelay(uint32_t ms);
void timerMsSleep(uint32_t ms);
bool timerMsElapsed(int64_t began, uint32_t ms);
uint32_t timerMsUntil(int64_t suppressionTimerMs);
bool timeSet(uint32_t newTimeSecs);
bool timeIsValid(void);
bool timeIsValidUnix(uint32_t t);
uint32_t timeSecs(void);
bool timeSecsElapsed(int64_t began, uint32_t secs);
void timeStr(uint32_t time, char *str, uint32_t length);
bool timeLocal(uint32_t time, uint32_t *year, uint32_t *mon1, uint32_t *day1, uint32_t *hour0, uint32_t *min0, uint32_t *sec0, uint32_t *wdaySun0);
void timeDateStr(uint32_t time, char *str, uint32_t length);
void timeSecondsToHMS(uint32_t sec, char *buf, uint32_t buflen);

// array.c
typedef void (*arrayEntryReset_t) (void *entry);
typedef bool (*arrayEntryIsLess_t) (void *a, void *b);
typedef struct {
    uint16_t count;
    uint16_t size;
    uint16_t chunksize;
    uint16_t cachedIndex;
    uint32_t length;
    uint32_t allocated;
    void *address;
    void *cachedAddress;
    arrayEntryReset_t resetFn;
    arrayEntryIsLess_t isLessFn;
} array;
#define arrayString array
typedef struct {
    uint16_t count;
    array *name;
    array *value1;
    array *value2;
} arrayMap;
#define arrayEntries(ctx) ((ctx)->count)
#define arrayLength(ctx) ((ctx)->length)
#define arrayAddress(ctx) ((ctx)->address)
#define arrayAllocString(x) arrayAlloc(0, NULL, x)
#define arrayAllocBytes(x) arrayAlloc(0, NULL, x)
err_t arrayInsert(array *ctx, uint16_t i, void *data);
void arrayRemove(array *ctx, uint16_t i);
err_t arraySet(array *ctx, int index, void *data);
void arrayShrink(array *ctx);
err_t arrayAppendBytes(array *ctx, void *data, uint16_t datalen);
err_t arrayAppendStringBytes(array *ctx, char *data);
err_t arrayAppendStringTerminate(array *ctx);
err_t arrayAppend(array *ctx, void *data);
void arrayResetEntry(array *ctx, int i);
void arrayReset(array *ctx);
err_t arrayAlloc(uint16_t fixedsize, arrayEntryReset_t entryReset, array **ctx);
err_t arrayDup(array *ctx, array **dupCtx);
void arrayFree(array *ctx);
void *arrayFreeDetach(array *ctx);
void *arrayFreeDetachDup(array *ctx);
err_t arraySort(array *ctx);
void *arrayEntry(array *ctx, int index);
err_t arrayMapAlloc(uint16_t fixedsize1, arrayEntryReset_t entryReset1, uint16_t fixedsize2, arrayEntryReset_t entryReset2, arrayMap **ctx);
void arrayMapShrink(arrayMap *map);
err_t arrayMapDup(arrayMap *in, arrayMap **out);
void arrayMapFree(arrayMap *map);
void arrayMapFreeDetachValue(arrayMap *map, array **value1, array **value2);
err_t arrayMapRemove(arrayMap *map, bool caseSensitive, char *name);
err_t arrayMapSet(arrayMap *map, bool caseSensitive, char *name, void *value1, void *value2);
bool arrayMapGet(arrayMap *map, bool caseSensitive, char *name, void *value1, void *value2);
bool arrayMapEntry(arrayMap *map, int index, char **name, void *value1, void *value2);
void arrayJoinString(array *ctx, char sep);
err_t arraySplitString(array *ctx, char sep);
void arrayClear(array *ctx);
void arrayMapClear(arrayMap *ctx);

// float16.c
typedef uint16_t float16_t;
float16_t float16FromFloat32(float x);
float float16ToFloat32(float16_t x);

// md5hash.c
#define MD5_SIZE 16
#define MD5_HEX_SIZE 32
#define MD5_HEX_STRING_SIZE (MD5_HEX_SIZE+1)
void md5Hash(uint8_t* data, uint32_t len, uint8_t *retHash);
void md5HashToString(uint8_t *data, uint32_t len, char *strbuf, uint32_t buflen);
void md5BinaryToString(uint8_t *hash, char *strbuf, uint32_t buflen);

// rand.c
unsigned long int randNumber(void);
unsigned long int randNumberBuffer(unsigned char* output, uint32_t sz);
unsigned long int prandNumber(void);
uint32_t randEvenDistribution(uint32_t n);
#define prandUint64() ((((uint64_t)prandNumber()) << 32LL) | ((uint64_t)prandNumber()))

// task.c
void taskRegister(int taskID, char *name, char letter, uint32_t stackBytes);
void taskRegisterAsNonBlocking(int taskID);
char *taskLabel(int taskID);
char taskIdentifier(void);
int taskID(void);
uint32_t taskAllIdleForMs(bool trace);
#define TASK_WAIT_FOREVER 0xffffffff
bool taskTake(int taskID, uint32_t timeoutMs);
void taskGive(int taskID);
void taskGiveFromISR(int taskID);
void taskGiveAllFromISR(void);
void taskSuspend(void);
void taskResume(void);
void taskStackStats();
uint32_t taskIOPriorityBegin();
void taskIOPriorityEnd(uint32_t prio);
void taskStackOverflowCheck();

// tss.c
void tssPreSuspend(int tssID);
void tssPostSuspend(int tssID);
void tssPause(void);
void tssResume(void);
void tssStats(void);

// memmem.c
void *memmem(const void *h0, size_t k, const void *n0, size_t l);

// crc16.c
uint16_t crc16(uint8_t const *data, size_t size);

// crc32.c
int32_t crc32(const void* data, size_t length);

// Virtual/Physical LED definitions, which may represent physical or neopixel LEDs
#define VLED_YELLOW         0
#define VLED_GREEN          1
#define VLED_RED            2
#define VLED_BLUE           3
#define VLED_CYAN           4
#define VLED_MAGENTA        5
#define VLED_WHITE          6
#define VLED_GRAY           7
#define VLED_ORANGE         8
#define VLED_TYPES          9
#define VLED_UNKNOWN        99

// base64.c
int Base64encode_len(int len);
int Base64encode(char * coded_dst, const char *plain_src,int len_plain_src);
int Base64decode_len(const char * coded_src);
int Base64decode(char * plain_dst, const char *coded_src, int *);

// neo.c
uint8_t neoCIColor(uint32_t vcolor);
void neoPulseTest(int testIterations);
err_t neoDisplayPattern(uint8_t *pattern, uint32_t patternLen, uint16_t repeatOverride);
void neoConfigureVLED(uint8_t *priorityTable, uint8_t *muxTable, uint32_t count, uint32_t userLED);
void neoVLED(uint32_t led, bool on);
void neoUserVLED(uint8_t ciColor);
err_t neoSetBrightness(uint32_t level);

// neotest.c
void neoTest(char *whichDemo, int iterations);

// os.c
uint32_t osBuildNum(void);
char *osBuildConfig(void);
bool osDebugging(void);
bool osUsbDetected(void);
bool streqlCI(const char *a, const char *b);
bool memeqlCI(void *av, void *bv, int len);
uint64_t atoh(char *p, int maxlen);
void stoh(char *src, uint8_t *dst, uint32_t dstlen);
void htoa32(uint32_t n, char *p);
void htoa16(uint16_t n, unsigned char *p);
void htoa8(uint8_t n, unsigned char *p);

// ERRORS
#define ERR_MEM_ALLOC "{memory}"
