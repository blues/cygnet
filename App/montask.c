
#include "app.h"

// Monitor task
void monTask(void *params)
{

    // Init task
    taskRegister(TASKID_MON, TASKNAME_MON, TASKLETTER_MON, TASKSTACK_MON);

    // Loop forever, doing monitor work
    while (true) {

        uint32_t pollMs = monitor();
        taskTake(TASKID_MON, pollMs);

    }

}
