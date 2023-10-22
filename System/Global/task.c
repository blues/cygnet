// Copyright 2017 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "app.h"
#include "global.h"
#include "gmutex.h"

// The mutex to protect event queues
STATIC mutex queueMutex = {MTX_QUEUE, {0}};

// Under FreeRTOS, the task contexts are task handles
STATIC TaskHandle_t taskHandle[TASKID_NUM_TASKS] = {0};
STATIC int64_t takeTimeoutDueMs[TASKID_NUM_TASKS] = {0};
STATIC char *taskName[TASKID_NUM_TASKS] = {0};
STATIC char taskLetter[TASKID_NUM_TASKS] = {0};
STATIC uint32_t taskStackBytes[TASKID_NUM_TASKS] = {0};
STATIC bool taskNoBlock[TASKID_NUM_TASKS] = {0};


// Set a task as doing its own blocking as opposed to using taskTake for blocks
void taskRegisterAsNonBlocking(int taskID)
{
    taskNoBlock[taskID] = true;
}

// Register a task's context
void taskRegister(int taskID, char *name, char letter, uint32_t stackBytes)
{
    taskHandle[taskID] = xTaskGetCurrentTaskHandle();
    taskName[taskID] = name;
    taskLetter[taskID] = letter;
    taskStackBytes[taskID] = stackBytes;
}

// Get a task's name
char *taskLabel(int taskID)
{
    char *label = taskName[taskID];
    return label == NULL ? "" : label;
}

// Get a task's identifying character
char taskIdentifier(void)
{
    char taskChar = '?';
    TaskHandle_t currentTaskHandle = xTaskGetCurrentTaskHandle();
    for (int task=0; task<TASKID_NUM_TASKS; task++) {
        if (taskHandle[task] == currentTaskHandle) {
            taskChar = taskLetter[task];
            break;
        }
    }
    return taskChar;
}

// Get a task's ID.
int taskID(void)
{
    int id = 0;
    TaskHandle_t currentTaskHandle = xTaskGetCurrentTaskHandle();
    for (int task=0; task<TASKID_NUM_TASKS; task++) {
        if (taskHandle[task] == currentTaskHandle) {
            return task;
        }
    }
    // Defensive programming only.
    debugPanic("task not found");
    return id;
}

// Return the shortest number of milliseconds that it's safe to sleep, in absence of an interrupt,
// based upon the timeouts on the taskTake()'s that are in-progress.
uint32_t taskAllIdleForMs(bool trace)
{
    int myTaskID = taskID();
    int64_t nowMs = timerMs();
    int64_t firstDueMs = 0;
    bool somethingRunning = false;
    char buf[32], status[256];
    if (trace) {
        snprintf(status, sizeof(status), "%c:run", taskLetter[myTaskID]);
    }
    for (int i=0; i<TASKID_NUM_TASKS; i++) {
        // Skip tasks that aren't initialized, and skip our task because obviously we're running
        if (taskNoBlock[i] || taskName[i] == NULL || i == myTaskID) {
            continue;
        }
        // If we find something running or which is already due, we're done
        int64_t taskDueMs = takeTimeoutDueMs[i];
        if (taskDueMs == 0) {
            somethingRunning = true;
            if (trace) {
                snprintf(buf, sizeof(buf), " %c:run", taskLetter[i]);
                strLcat(status, buf);
            } else {
                break;
            }
        } else if (taskDueMs < nowMs) {
            somethingRunning = true;
            if (trace) {
                snprintf(buf, sizeof(buf), " %c:due", taskLetter[i]);
                strLcat(status, buf);
            } else {
                break;
            }
        } else {
            if (firstDueMs == 0 || taskDueMs < firstDueMs) {
                firstDueMs = taskDueMs;
            }
            if (trace) {
                uint32_t ms = (uint32_t) (taskDueMs - nowMs);
                if (ms > ms1Sec) {
                    snprintf(buf, sizeof(buf), " %c:%lds", taskLetter[i], (long) (ms/ms1Sec));
                } else {
                    snprintf(buf, sizeof(buf), " %c:%ldms", taskLetter[i], (long) ms);
                }
                strLcat(status, buf);
            }
        }
    }
    uint32_t dueMs = 0;
    if (!somethingRunning && firstDueMs > nowMs) {
        dueMs = (uint32_t) (firstDueMs - nowMs);
    }
    if (trace) {
        debugR("NEXT:%ldms %s\n", (long) dueMs, status);
    }
    return dueMs;
}

// Take from the task's event semaphore.  This should be a counting semaphore so that we avoid any
// race conditions between a take wakeup and the next take.  Return false if timeout.
bool taskTake(int taskID, uint32_t timeoutMs)
{

    // Don't rely upon RTOS semantics, where sometimes 0 means "infinite"
    if (timeoutMs == 0) {
        timeoutMs = 1;
    }

    // Forever
    if (timeoutMs == TASK_WAIT_FOREVER) {
        timeoutMs = portMAX_DELAY;
    }

    // Defensive coding for init
    if (taskHandle[taskID] == 0) {
        return true;
    }

    // Pause
    tssPreSuspend(taskID);
    takeTimeoutDueMs[taskID] = timerMs() + (int64_t) timeoutMs;
    bool timeout = (ulTaskNotifyTake(pdFALSE, timeoutMs) == 0);
    takeTimeoutDueMs[taskID] = 0;
    tssPostSuspend(taskID);

    // Done
    return !timeout;
}

// Give to an event's semaphore, but not from an ISR
void taskGive(int taskID)
{
    if (taskHandle[taskID] == 0) {
        return;
    }
    TaskHandle_t hTask = taskHandle[taskID];
    if (hTask != xTaskGetCurrentTaskHandle()) {
        xTaskNotifyGive(hTask);
    }
}

// Give to all tasks from an ISR
void taskGiveAllFromISR(void)
{
    for (int task=0; task<TASKID_NUM_TASKS; task++) {
        taskGiveFromISR(task);
    }
}

// Give to an event's semaphore from an ISR, and allow the scheduler to naturally determine what
// task is next to be scheduled given task priorities as they are.
void taskGiveFromISR(int taskID)
{
    if (taskHandle[taskID] == 0) {
        return;
    }
    vTaskNotifyGiveFromISR(taskHandle[taskID], NULL);
}

// Terminate all task scheduling
void taskSuspend()
{
    vTaskSuspendAll();
}

// Resume task scheduling.
void taskResume()
{
    xTaskResumeAll();
}

// Check stack stats
void taskStackStats()
{

    debugR("task stacks:\n");

    for (int task=0; task<TASKID_NUM_TASKS; task++) {
        TaskHandle_t hTask = taskHandle[task];
        if (hTask != 0) {
            TaskStatus_t status;
            vTaskGetInfo(hTask, &status, pdTRUE, eInvalid);
            double pct = (double)(status.usStackHighWaterMark*sizeof(StackType_t))/(double)taskStackBytes[task];
            debugR("  %12s: %lu/%lu %0.2f%% remaining\n", status.pcTaskName, status.usStackHighWaterMark*sizeof(StackType_t), taskStackBytes[task], pct*100.0);
            if (status.usStackHighWaterMark < STACKWORDS(500)) {
                for (int i=0; i<8; i++) {
                    debugf("*****************************************************\n");
                    if (i == 4) {
                        debugf("***** NEAR STACK OVERFLOW!!!\n");
                    }
                }
            }
        }
    }

    debugR("\n");

}

// Temporarily increase the task's priority to maximum
uint32_t taskIOPriorityBegin()
{
    TaskHandle_t task = xTaskGetCurrentTaskHandle();
    uint32_t prio = uxTaskPriorityGet(task);
    vTaskPrioritySet(task, configMAX_PRIORITIES-1);
    return prio;
}

// Restore the task's priority
void taskIOPriorityEnd(uint32_t prio)
{
    vTaskPrioritySet(xTaskGetCurrentTaskHandle(), prio);
}

// Check stack to ensure that we're not near overflow
void taskStackOverflowCheck()
{

    for (int task=0; task<TASKID_NUM_TASKS; task++) {
        TaskHandle_t hTask = taskHandle[task];
        if (hTask != 0) {
            TaskStatus_t status;
            vTaskGetInfo(hTask, &status, pdTRUE, eInvalid);
            if (status.usStackHighWaterMark < 175) {
                static char message[80];
                snprintf(message, sizeof(message), "STACK: %s: %ld remaining (%ld words)", status.pcTaskName, (long) (status.usStackHighWaterMark*sizeof(StackType_t)), (long) status.usStackHighWaterMark);
                debugPanic(message);
            }
        }
    }

}

// Initialize a binary event
void taskEventInit(taskEvent *event, bool signalled)
{
    mutexInit(&event->waiting, MTX_EVENT);
    event->binarySem = xSemaphoreCreateBinary();
    event->signalled = signalled;
    if (signalled) {
        xSemaphoreGive(event->binarySem);
    }
}

// De-initialize a binary event
void taskEventDeInit(taskEvent *event)
{
    vSemaphoreDelete(event->binarySem);
    mutexDeInit(&event->waiting);
}

// Wait for the earlier of a signal or timeout, returning true if timeout
bool taskEventWait(taskEvent *event, int timeoutMs)
{
    mutexLock(&event->waiting);
    int64_t expiresMs = timerMs() + timeoutMs;
    while (!event->signalled) {
        int64_t now = timerMs();
        if (now >= expiresMs) {
            mutexUnlock(&event->waiting);
            return true;
        }
        xSemaphoreTake(event->binarySem, (int) (expiresMs - now));
    }
    event->signalled = false;
    mutexUnlock(&event->waiting);
    return false;
}

// Signal an event
void taskEventSignal(taskEvent *event)
{
    event->signalled = true;
    xSemaphoreGive(event->binarySem);
}

// Allocate a queue, returning true if success
bool taskQueueAlloc(int bytesPerEntry, int numEntries, taskQueue **pQueue)
{
    taskQueue *q;
    err_t err = memAlloc(sizeof(taskQueue)+(bytesPerEntry * numEntries), &q);
    if (err) {
        return false;
    }
    q->totalEntries = numEntries;
    q->entryBytes = bytesPerEntry;
    taskEventInit(&q->getWait, false);
    taskEventInit(&q->putWait, false);
    *pQueue = q;
    return true;
}

// Free a queue, returning true if success
void taskQueueFree(taskQueue *pQueue)
{

    // Force it to exit
    pQueue->totalEntries = 0;
    taskQueueWake(pQueue);

    // Deinit and free
    taskEventDeInit(&pQueue->getWait);
    taskEventDeInit(&pQueue->putWait);
    memFree(pQueue);

}

// See how many entries are pending
int taskQueuePending(taskQueue *queue)
{
    return queue->currentEntries;
}

// Goose a queue to see if we can get the other guy to process it
void taskQueueWake(taskQueue *queue)
{
    taskEventSignal(&queue->getWait);
    taskEventSignal(&queue->putWait);
    timerMsSleep(100);
}

// Pull from the task queue
bool taskQueueGet(taskQueue *queue, void *data, uint32_t waitMs)
{

    // Lock while processing
    int64_t expiresMs = timerMs() + waitMs;
    uint8_t *qdata = (uint8_t *) (&queue[1]);
    mutexLock(&queueMutex);
    while (true) {

        // Pull off the next queue entry
        if (queue->currentEntries > 0) {
            queue->currentEntries--;
            memcpy((uint8_t *)data, qdata + (queue->entryBytes * queue->drain), queue->entryBytes);
            queue->drain++;
            if (queue->drain >= queue->totalEntries) {
                queue->drain = 0;
            }
            mutexUnlock(&queueMutex);
            taskEventSignal(&queue->putWait);
            return true;
        }

        // Look like timeout if shutdown in progress
        if (queue->totalEntries == 0) {
            break;
        }

        // Exit if timeout
        int64_t now = timerMs();
        if (now >= expiresMs) {
            break;
        }

        // Wait for an entry to be enqueued
        mutexUnlock(&queueMutex);
        taskEventWait(&queue->getWait, (int) (expiresMs - now));
        mutexLock(&queueMutex);

    }

    // Timeout
    mutexUnlock(&queueMutex);
    return false;

}

// Put an entry into the task queue
bool taskQueuePut(taskQueue *queue, void *data, uint32_t waitMs)
{

    // Lock while processing
    int64_t expiresMs = timerMs() + waitMs;
    void *qdata = (void *) (&queue[1]);
    mutexLock(&queueMutex);
    while (true) {

        // Look like timeout if shutdown in progress
        if (queue->totalEntries == 0) {
            break;
        }

        // Put it onto the queue
        if (queue->currentEntries < queue->totalEntries) {
            memcpy((uint8_t *)qdata + (queue->entryBytes * queue->fill), (uint8_t *)data, queue->entryBytes);
            queue->fill++;
            if (queue->fill >= queue->totalEntries) {
                queue->fill = 0;
            }
            queue->currentEntries++;
            mutexUnlock(&queueMutex);
            taskEventSignal(&queue->getWait);
            return true;
        }

        // Exit if timeout
        int64_t now = timerMs();
        if (now >= expiresMs) {
            break;
        }

        // Wait for an entry to be enqueued
        mutexUnlock(&queueMutex);
        taskEventWait(&queue->putWait, (int) (expiresMs - now));
        mutexLock(&queueMutex);

    }

    // Timeout
    mutexUnlock(&queueMutex);
    return false;

}
