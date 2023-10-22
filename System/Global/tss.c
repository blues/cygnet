// Copyright 2017 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "main.h"
#include "app.h"
#include "global.h"

STATIC bool paused = false;
STATIC int64_t tssBeganMs = 0;
STATIC int64_t tssEndedMs = 0;
STATIC uint32_t taskSwitches[TASKID_NUM_TASKS];
STATIC int64_t taskResumedMs[TASKID_NUM_TASKS];
STATIC int64_t taskTotalMs[TASKID_NUM_TASKS];

// Remember how long we were running
void tssPreSuspend(int tssID)
{
    int64_t runningMs = timerMs() - taskResumedMs[tssID];
    if (runningMs <= 0) {
        taskTotalMs[tssID]++;
    } else {
        taskTotalMs[tssID] += runningMs;
    }
}

// Bump the count associated with a task
void tssPostSuspend(int tssID)
{
    if (tssBeganMs == 0) {  // one-time init
        tssResume();
    }
    if (!paused) {
        taskSwitches[tssID]++;
    }
    taskResumedMs[tssID] = timerMs();
}

// Pause stats gathering
void tssPause()
{
    tssEndedMs = timerMs();
    paused = true;
}

// Reset stats
void tssResume()
{
    memset(taskSwitches, 0, sizeof(taskSwitches));
    memset(taskTotalMs, 0, sizeof(taskTotalMs));
    tssBeganMs = timerMs();
    paused = false;
}

// Dump stats about task switches
void tssStats()
{
    if (paused) {
        int64_t durationMs = (tssEndedMs - tssBeganMs);
        debugR("Task Switch Rate over prev %d seconds:\n", (uint32_t)(durationMs/1000));
        for (int task=0; task<TASKID_NUM_TASKS; task++) {
            uint32_t switches = taskSwitches[task];
            taskSwitches[task] = 0;
            double rate = (((double) switches) / durationMs) * ms1Min;
            char *label = taskLabel(task);
            if (label[0] != '\0') {
                debugR("%12s: %.2f/min %ld ms\n", label, rate, (long) taskTotalMs[task]);
            }
        }
        debugR("\n");
    }
}
