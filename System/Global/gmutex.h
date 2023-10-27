// Copyright 2017 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "global.h"
#include <stdatomic.h>

#pragma once

// This file was split out from global.h because its included headers are substantial and
// thus by splitting it out there is a significant speedup of builds.
#define mutexTrace              true        // Leave on in production because the cost is very low

// Mutex definitions, low-order to high-order in order of layering - for deadlock detection.
// These values can be changed as necessary when adding new ones, however be very careful because
// this does define the one and only nested layering of semaphores necessary to prevent deadlock.
// Note that we assign these with 255 possible mutexes that may be inserted "between" them
// so that apps using app_mutex.h may insert values without editing this file.
#define MTX_MTX         0x0000000000000100
#define MTX_TIME        0x0000000000000200
#define MTX_EVENT       0x0000000000000400
#define MTX_QUEUE       0x0000000000000800
#define MTX_RAND        0x0000000000001000
#define MTX_ERR         0x0000000000002000
#define MTX_SERIAL_TX   0x0000000000004000
#define MTX_SERIAL_RX   0x0000000000008000
typedef uint64_t mtxtype_t;

// mutex.c
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
// Note that all mutexes in the product MUST BE STATIC and initialized to {0}, for both RTOS and GLIB
typedef struct {
    mtxtype_t mtx;
    struct {
        volatile bool initialized;
        SemaphoreHandle_t handle;
#if mutexTrace
        const char *filename;
        uint32_t lineno;
#endif
        // Nonzero if locked, zero if locked
        int64_t lockedMs;
        // For internal consistency validation
        int lockedTask;
    } state;
} mutex;
#if mutexTrace
#define mutexLock(x) mutexLockHandler(__FILE__, __LINE__, x)
void mutexLockHandler(const char *filename, uint32_t lineno, mutex *m);
#else
void mutexLock(mutex *m);
#endif
void mutexUnlock(mutex *m);
bool mutexIsLocked(mutex *m);
bool mutexIsLockedByThisTask(mutex *m);
void mutexShow(mutex *m);
void mutexInit(mutex *m, mtxtype_t mtype);
void mutexDeInit(mutex *m);
void mutexDebugOwned(const char **name, int64_t *owned);

// Events
typedef struct {
    mutex waiting;
    SemaphoreHandle_t binarySem;
    bool signalled;
} taskEvent;
void taskEventInit(taskEvent *event, bool presignalled);
void taskEventDeInit(taskEvent *event);
bool taskEventWait(taskEvent *event, int timeoutMs);
void taskEventSignal(taskEvent *event);

// Queues
typedef struct {
    int fill;
    int drain;
    int currentEntries;
    int totalEntries;
    int entryBytes;
    taskEvent getWait;
    taskEvent putWait;
} taskQueue;
bool taskQueueAlloc(int bytesPerEntry, int numEntries, taskQueue **pQueue);
int taskQueuePending(taskQueue *queue);
void taskQueueWake(taskQueue *queue);
void taskQueueFree(taskQueue *queue);
bool taskQueuePut(taskQueue *queue, void *data, uint32_t waitMs);
bool taskQueueGet(taskQueue *queue, void *data, uint32_t waitMs);

// App's overrides to gmutex.h, which are resolved in the "weak" folder if the app doesn't define them
#include "app_mutex.h"
