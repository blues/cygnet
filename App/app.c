
#include "main.h"
#include "app.h"
#include "rtc.h"
#include "stm32_lpm_if.h"

// When we went to sleep
STATIC int64_t sleepBeganMs = 0;

// Initialize the app
void appInit()
{

    // Create the main task
    xTaskCreate(mainTask, TASKNAME_MAIN, STACKWORDS(TASKSTACK_MAIN), NULL, TASKPRI_MAIN, NULL);

}

// ISR for interrupts to be processed by the app
void appISR(uint16_t GPIO_Pin)
{

    // Button processing, noting that the button is ACTIVE LOW
#ifdef USER_BTN_Pin
    if ((GPIO_Pin & USER_BTN_Pin) != 0) {
        buttonPressISR(HAL_GPIO_ReadPin(USER_BTN_GPIO_Port, USER_BTN_Pin) == GPIO_PIN_RESET);
    }
#endif

    // USB Detect processing, noting that the signal is ACTIVE HIGH
#ifdef USB_DETECT_Pin
    if ((GPIO_Pin & USB_DETECT_Pin) != 0) {
        serialResetUSB();
        taskGiveAllFromISR();
    }
#endif

}

// See FreeRTOSConfig.h where this is registered via configPRE_SLEEP_PROCESSING()
// Called by the kernel before it places the MCU into a sleep mode because
void appPreSleepProcessing(uint32_t ulExpectedIdleTime)
{

    // NOTE:  Additional actions can be taken here to get the power consumption
    // even lower.  For example, peripherals can be turned off here, and then back
    // on again in the post sleep processing function.  For maximum power saving
    //  ensure all unused pins are in their lowest power state.

    // Remember when we went to sleep
    sleepBeganMs = MX_RTC_GetMs();

    // Enter to sleep Mode using the HAL function HAL_PWR_EnterSLEEPMode with WFI instruction
    UTIL_PowerDriver.EnterStopMode();

}

// RTC heartbeat, used for timeouts and watchdogs
void appHeartbeatISR(uint32_t heartbeatSecs)
{
}

// See FreeRTOSConfig.h where this is registered via configPOST_SLEEP_PROCESSING()
// Called by the kernel before it places the MCU into a sleep mode because
void appPostSleepProcessing(uint32_t ulExpectedIdleTime)
{

    (void) ulExpectedIdleTime;

    // Tell FreeRTOS how long we were asleep.  Note that we don't
    // ever steptick by 1ms because that's what we see when there is
    // no actual sleep performed, which has the net effect of clocking
    // the timer faster than it's actually supposed to go.
    int64_t elapsedMs = MX_RTC_GetMs() - sleepBeganMs;
    if (sleepBeganMs != 0 && elapsedMs > 1) {
        sleepBeganMs = 0;
        vTaskStepTick(pdMS_TO_TICKS(elapsedMs-1));
        MX_StepTickMs(elapsedMs);
    }

    // Exit stop mode
    UTIL_PowerDriver.ExitStopMode();

}

// Initialize the app's GPIOs
void appInitGPIO(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    // Initialize floats
    memset(&GPIO_InitStruct, 0, sizeof(GPIO_InitStruct));
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;

    GPIO_InitStruct.Pin = MODEM_RESET_Pin;
    HAL_GPIO_Init(MODEM_RESET_GPIO_Port, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = MODEM_PWRKEY_Pin;
    HAL_GPIO_Init(MODEM_PWRKEY_GPIO_Port, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = MODEM_PSM_WAKE_Pin;
    HAL_GPIO_Init(MODEM_PSM_WAKE_GPIO_Port, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = MODEM_DISCHARGE_Pin;
    HAL_GPIO_Init(MODEM_DISCHARGE_GPIO_Port, &GPIO_InitStruct);

    // Initialize outputs
    memset(&GPIO_InitStruct, 0, sizeof(GPIO_InitStruct));
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    GPIO_InitStruct.Pin = LED_BUSY_Pin;
    HAL_GPIO_Init(LED_BUSY_GPIO_Port, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LED_BUSY_GPIO_Port, LED_BUSY_Pin, GPIO_PIN_RESET);

    GPIO_InitStruct.Pin = MODEM_POWER_Pin;
    HAL_GPIO_Init(MODEM_POWER_GPIO_Port, &GPIO_InitStruct);
    HAL_GPIO_WritePin(MODEM_POWER_GPIO_Port, MODEM_POWER_Pin, GPIO_PIN_RESET);

    // Initialize inputs as floats for until we are ready to use them
    memset(&GPIO_InitStruct, 0, sizeof(GPIO_InitStruct));
#if 0
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pin = MODEM_RI_Pin;
    HAL_GPIO_Init(MODEM_RI_GPIO_Port, &GPIO_InitStruct);
    HAL_NVIC_SetPriority(MODEM_RI_IRQn, INTERRUPT_PRIO_EXTI, 0);
    HAL_NVIC_EnableIRQ(MODEM_RI_IRQn);
#else
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Pin = MODEM_RI_Pin;
    HAL_GPIO_Init(MODEM_RI_GPIO_Port, &GPIO_InitStruct);
#endif


}
