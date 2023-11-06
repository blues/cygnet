// Copyright 2017 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "global.h"
#include "app.h"
#include "mutex.h"
#include "main.h"

#define MUTEX_HELD_DURATION_WARNING_MS      500
#define MUTEX_NEEDED_DURATION_WARNING_MS    100
#define SHOW_MUTEX_DURATION_WARNINGS        false
#define SHOW_MUTEX_LOCKS                    false
#define debugMessage(x)                     MX_DBG(x, strlen(x))

// Note that we cannot use these in here because we'd go recursive
#undef debugf
#define debugf DO-NOT-USE
#undef debugR
#define debugR DO-NOT-USE

// For task handle
#include "task.h"

// For deadlock checking
#if mutexTrace
STATIC mtxtype_t taskMutexes[TASKID_NUM_TASKS] = {0};
#endif

// The mutex to protect mutex creation
STATIC mutex initMutex = {MTX_MTX, {0}};

// Forwards
char *justFilename(const char *fileName);

// Init a mutex
void mutexInit(mutex *m, mtxtype_t mtype)
{
    memset(m, 0, sizeof(mutex));
    m->mtx = mtype;
}

// For debugging, display which mutexes are owned by the current task.  Note that
// we can do this without locking any mutexes because we are only looking at
// the state as it pertains to the current task.
void mutexDebugOwned(const char **name, int64_t *owned)
{
    *owned = 0;
    *name = "";
#if mutexTrace
    *name = taskLabel(taskID());
    *owned = (int64_t) taskMutexes[taskID()];
#endif
}

// DeInit a mutex
void mutexDeInit(mutex *m)
{
    if (m->state.initialized) {
        vSemaphoreDelete(m->state.handle);
    }
}

// Lock a resource
#if mutexTrace
void mutexLockHandler(const char *filename, uint32_t lineno, mutex *m)
#else
void mutexLock(mutex *m)
#endif
{
    int thisTaskID = taskID();

    // First time through ANY mutex lock?
    if (!initMutex.state.initialized) {
        initMutex.state.handle = xSemaphoreCreateMutex();
        if (initMutex.state.handle == NULL) {
            debugPanic("can't allocate init mutex");
        }
        initMutex.state.initialized = true;
    }

    // First time through this mutex?
    if (!m->state.initialized) {
        xSemaphoreTake(initMutex.state.handle, portMAX_DELAY);
        if (!m->state.initialized) {
            m->state.handle = xSemaphoreCreateMutex();
            if (m->state.handle == NULL) {
                debugPanic("can't allocate mutex");
            }
            m->state.lockedTask = -1;
            m->state.initialized = true;
        }
        xSemaphoreGive(initMutex.state.handle);
    }

    // Validate that we're not locking nested
    if (m->state.lockedTask == thisTaskID) {
#if mutexTrace
        char reason[128];
        snprintf(reason, sizeof(reason), "*** mutexLock nested! %s:%u\n", justFilename(m->state.filename), (unsigned)m->state.lineno);
        debugPanic(reason);
#else
        debugPanic("*** mutexLock Nested!");
#endif
    }

    // Take the mutex
#if mutexTrace && SHOW_MUTEX_DURATION_WARNINGS
    int64_t timerBegan = timerMs();
    while (!xSemaphoreTake(m->state.handle, MUTEX_NEEDED_DURATION_WARNING_MS)) {
        char reason[128];
        uint32_t secsHeld = (uint32_t) (timerMs() - timerBegan)/1000;
        snprintf(reason, sizeof(reason), "$$$$ mutex needed by %s:%u (%d) is being held for %us by %s:%u (%d)\n", justFilename(filename), (unsigned)lineno, taskID(), secsHeld, justFilename(m->state.filename), m->state.lineno, m->state.lockedTask);
        debugMessage(reason);
    }
#else
    xSemaphoreTake(m->state.handle, portMAX_DELAY);
#endif

    // Trace
#if SHOW_MUTEX_LOCKS
    char reason[128];
    snprintf(reason, sizeof(reason), "mutexLock: %s 0x%016llx %s:%u\n", taskLabel(thisTaskID), (unsigned long long)m->mtx, justFilename(filename), (unsigned)lineno);
    debugMessage(reason);
#endif

    // Remember the state of the underlying mutex, for debugging
    m->state.lockedTask = thisTaskID;
    m->state.lockedMs = timerMs();

    // Do mutex ordering checking.  Note that we do allow mutex type to be 0 because
    // external packages such as lwip create mutexes and manage their own nesting.
#if mutexTrace
    if (m->mtx != 0) {
        mtxtype_t lowerLevelMask = m->mtx - 1;
        if (taskMutexes[thisTaskID] & lowerLevelMask) {
            char reason[128];
            snprintf(reason, sizeof(reason), "*** %s mutex ordering (want 0x%016llx while holding 0x%016llx) %s:%u\n", taskLabel(thisTaskID), (unsigned long long)m->mtx, (unsigned long long)taskMutexes[thisTaskID], justFilename(filename), (unsigned)lineno);
            debugSoftPanic(reason);
        }
        taskMutexes[thisTaskID] |= m->mtx;
    }
#endif

    // Remember who grabbed it
#if mutexTrace
    m->state.filename = filename;
    m->state.lineno = lineno;
#endif

}

// Test OPPORTUNISTICALLY to see if this mutex is currently locked.  This is used ONLY when there are
// multiple options for "work to do" and it's preferable to do something that you know would cause a
// block.  This is used by the serial poll task, where other tasks may have receive buffers locked
// because of work in progress.
bool mutexIsLocked(mutex *m, int *lockedTaskID)
{
    if (m->state.lockedTask != -1 && m->state.lockedTask != taskID()) {
        if (lockedTaskID != NULL) {
            *lockedTaskID = m->state.lockedTask;
        }
        return true;
    }
    return false;
}

// Unlock the resource
void mutexUnlock(mutex *m)
{

    // Trace
#if SHOW_MUTEX_LOCKS
    char reason[128];
    snprintf(reason, sizeof(reason), "mutexUnlock: %s 0x%016llx\n", taskLabel(taskID()), (unsigned long long)m->mtx);
    debugMessage(reason);
#endif

    if (m->state.lockedTask == -1) {
#if mutexTrace
        char reason[128];
        snprintf(reason, sizeof(reason), "*** mutexUnlock of unlocked mutex!  %s:%u\n", justFilename(m->state.filename), (unsigned)m->state.lineno);
        debugPanic(reason);
#else
        debugPanic("*** mutexUnlock of unlocked mutex!");
#endif
    }
    if (m->state.lockedTask != taskID()) {
#if mutexTrace
        char reason[128];
        snprintf(reason, sizeof(reason), "*** mutexUnlock of mutex owned by another task! (cur:%d owner:%d)  %s:%u\n", taskID(), m->state.lockedTask, justFilename(m->state.filename), (unsigned)m->state.lineno);
        debugPanic(reason);
#else
        debugPanic("*** mutexUnlock of mutex owned by another task!");
#endif
    }
#if mutexTrace && SHOW_MUTEX_DURATION_WARNINGS
    int64_t xLockedMs = 0;
    const char *xFilename = m->state.filename;
    uint32_t xLineno = m->state.lineno;
    xLockedMs = m->state.lockedMs;
#endif
    m->state.lockedTask = -1;
    m->state.lockedMs = 0;
#if mutexTrace
    taskMutexes[taskID()] &= ~m->mtx;
#endif

#if mutexTrace
    m->state.filename = "";
    m->state.lineno = 0;
#endif

    xSemaphoreGive(m->state.handle);

#if mutexTrace && SHOW_MUTEX_DURATION_WARNINGS
    if (xLockedMs != 0) {
        int64_t msElapsed = timerMs() - xLockedMs;
        if (msElapsed > MUTEX_HELD_DURATION_WARNING_MS) {
            if (m->mtx != MTX_EVENT) {      // Ignore simple waits for queue or timerMsSleep()
                char *header = "=====\n";
                char reason[128];
                snprintf(reason, sizeof(reason), "===== mutexUnlock: locked for %dms %s:%u\n", (int) msElapsed, justFilename(xFilename), (unsigned)xLineno);
                debugMessage(header);
                debugMessage(header);
                debugMessage(reason);
                debugMessage(header);
                debugMessage(header);
            }
        }
    }
#endif

}

// Return a pointer to the filename portion of a path.  Do NOT use mutexes, because this is
// too low level and is called by the mutex code itself.
char *justFilename(const char *fileName)
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
