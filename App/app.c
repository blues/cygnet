// Copyright 2024 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "main.h"
#include "app.h"
#include "rtc.h"
#include "stm32_lpm_if.h"

// Initialize the app
void appInit()
{

    // Create the main task
    xTaskCreate(mainTask, TASKNAME_MAIN, STACKWORDS(TASKSTACK_MAIN), NULL, TASKPRI_MAIN, NULL);

}

// Initialize the app's GPIOs
void appInitGPIO(void)
{
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
    // We just wake up any task that might be interested in the change
#ifdef USB_DETECT_Pin
    if ((GPIO_Pin & USB_DETECT_Pin) != 0) {
        taskGiveAllFromISR();
    }
#endif

}

// RTC heartbeat, used for timeouts and watchdogs
void appHeartbeatISR(uint32_t heartbeatSecs)
{
}

// The time when we went into STOP2 mode
int64_t sleepBeganMs = 0;

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

    // Enter to sleep Mode using the HAL function HAL_PWR_EnterSLEEPMode with WFI instruction.
    // Note that this MAY or MAY NOT enter stop mode depending upon factors that you
    // can see in PWR_EnterStopMode() in stm32_lpm_if.c which is called inside this method.
    UTIL_PowerDriver.EnterStopMode();

    // Note that if we get here we know that we did NOT enter stop mode, and we must
    // set this to 0 so that on post-sleep processing we don't step the tick
    if (!PWR_WasStopped()) {
        sleepBeganMs = 0;
    }

}

// See FreeRTOSConfig.h where this is registered via configPOST_SLEEP_PROCESSING()
// Called by the kernel before it places the MCU into a sleep mode because
void appPostSleepProcessing(uint32_t ulExpectedIdleTime)
{

    (void) ulExpectedIdleTime;

    // Exit stop mode
    UTIL_PowerDriver.ExitStopMode();

    // Tell FreeRTOS how long we were asleep.  Note that we don't
    // ever steptick by 1ms because that's what we see when there is
    // no actual sleep performed, which has the net effect of clocking
    // the timer faster than it's actually supposed to go.
    if (sleepBeganMs != 0) {
        int64_t elapsedMs = MX_RTC_GetMs() - sleepBeganMs;
        if (elapsedMs > 1) {
            vTaskStepTick(pdMS_TO_TICKS(elapsedMs-1));
            MX_StepTickMs(elapsedMs);
        }
    }

}

// Return true if sleep is allowed
bool appSleepAllowed(void)
{
    if (serialIsActive()) {
        return false;
    }
    if (osUsbDetected()) {
        return false;
    }
    return APP_SUPPORTS_STOP2_MODE;
}
