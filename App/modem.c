
#include "app.h"
#include "serial.h"
#include "usart.h"

// Forwards
bool processModemResponse(UART_HandleTypeDef *huart);

// Request task
void modemTask(void *params)
{

    // Init task
    taskRegister(TASKID_MODEM, TASKNAME_MODEM, TASKLETTER_MODEM, TASKSTACK_MODEM);

    // Loop, extracting requests from the serial port and processing them
    while (true) {
        bool didSomething = false;
        didSomething |= processModemResponse(&huart2);
        if (!didSomething) {
            taskTake(TASKID_MODEM, ms1Day);
        }
    }

}

// Process incoming serial request
bool processModemResponse(UART_HandleTypeDef *huart)
{

    // Get the pending JSON request
    uint8_t *modemResponse;
    uint32_t modemResponseLen;
    bool diagAllowed;
    if (!serialLock(huart, &modemResponse, &modemResponseLen, &diagAllowed)) {
        return false;
    }

    // If it's a 0-length request, we must output our standard \r\n response
    // because this is critical for note-c to answer "are you there?"
    if (modemResponseLen == 0) {
        serialUnlock(huart, true);
        return true;
    }

    // Process modem
    debugR("modem: \"%.*s\"\n", modemResponseLen, modemResponse);

    // Done
    serialUnlock(huart, true);
    return true;

}
