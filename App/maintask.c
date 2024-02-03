
#include "app.h"
#include "task.h"
#include "queue.h"
#include "stm32_lpm.h"
#include "timer_if.h"
#include "rtc.h"
#include "utilities_def.h"

// Main task
void mainTask(void *params)
{

    // Init task
    taskRegister(TASKID_MAIN, TASKNAME_MAIN, TASKLETTER_MAIN, TASKSTACK_MAIN);

    // Init low power manager
    UTIL_LPM_Init();
    UTIL_LPM_SetOffMode((1 << CFG_LPM_APPLI_Id), UTIL_LPM_DISABLE);
    if (osDebugging()) {
        UTIL_LPM_SetStopMode((1 << CFG_LPM_APPLI_Id), UTIL_LPM_DISABLE);
    }

    // Init timer manager
    UTIL_TIMER_Init();

    // Set defaults from how we were configured
    configGetDefaultsFromTestCert();

    // Remember the timer value when we booted
    timerSetBootTime();

    // Initialize serial, thus enabling debug output
    serialInit(TASKID_MAIN);

    // Initialize the modem package
    modemInit();

    // Enable debug info at startup, by policy
    MX_DBG_Enable(true);

    // Display usage stats
    debugf("\n");
    debugf("**********************\n");
    char timestr[48];
    uint32_t s = timeSecs();
    timeDateStr(s, timestr, sizeof(timestr));
    if (timeIsValid()) {
        debugf("%s UTC (%d)\n", timestr, s);
    }
    uint32_t pctUsed = ((heapPhysical-heapFreeAtStartup)*100)/65536;
    debugf("RAM: %lu free of %lu (%d%% used)\n", (unsigned long) heapFreeAtStartup, heapPhysical, pctUsed);
    extern void *ROM_CONTENT$$Limit;
    uint32_t imageSize = (uint32_t) (&ROM_CONTENT$$Limit) - FLASH_BASE;
    pctUsed = (imageSize*100)/262144;
    debugf("ROM: %lu free of 256KB (%d%% used)\n", (unsigned long) (262144 - imageSize), pctUsed);
    debugf("**********************\n");

    // Signal that we've started, but do it here so we don't block request processing
    ledRestartSignal();

    // Hang here if a DFU is in progress
    if (HAL_GPIO_ReadPin(EN_MODEM_DFU_GPIO_Port, EN_MODEM_DFU_Pin) == GPIO_PIN_SET) {
        HAL_GPIO_WritePin(MAIN_POWER_GPIO_Port, MAIN_POWER_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(SEL_SIM_GPIO_Port, SEL_SIM_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(MODEM_RESET_NOD_GPIO_Port, MODEM_RESET_NOD_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(MODEM_PWRKEY_NOD_GPIO_Port, MODEM_PWRKEY_NOD_Pin, GPIO_PIN_SET);
        timerMsSleep(100);
        HAL_GPIO_WritePin(MODEM_POWER_NOD_GPIO_Port, MODEM_POWER_NOD_Pin, GPIO_PIN_RESET);
        timerMsSleep(750);
        HAL_GPIO_WritePin(MODEM_PWRKEY_NOD_GPIO_Port, MODEM_PWRKEY_NOD_Pin, GPIO_PIN_RESET);
        timerMsSleep(1500);
        HAL_GPIO_WritePin(MODEM_PWRKEY_NOD_GPIO_Port, MODEM_PWRKEY_NOD_Pin, GPIO_PIN_SET);
        for (int i=1;; i++) {
            if (HAL_GPIO_ReadPin(EN_MODEM_DFU_GPIO_Port, EN_MODEM_DFU_Pin) != GPIO_PIN_SET) {
                break;
            }
            debugf("%04d **** MODEM FIRMWARE UPDATE-IN-PROGRESS SWITCH IS 'ON' ***\n", i);
            timerMsSleep(1000);
            ledBusy((i & 1) != 0);
        }
        debugPanic("firmware updated");
    }


    // Create the serial request processing task
    xTaskCreate(reqTask, TASKNAME_REQ, STACKWORDS(TASKSTACK_REQ), NULL, TASKPRI_REQ, NULL);

    // Create the serial modem processing task
    xTaskCreate(modemTask, TASKNAME_MODEM, STACKWORDS(TASKSTACK_MODEM), NULL, TASKPRI_MODEM, NULL);

    // Create the monitor task
    xTaskCreate(monTask, TASKNAME_MON, STACKWORDS(TASKSTACK_MON), NULL, TASKPRI_MON, NULL);

    // Poll, moving serial data from interrupt buffers to app buffers
    for (;;) {
        serialPoll();
    }

}
