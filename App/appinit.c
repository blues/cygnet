
#include "main.h"
#include "notelib.h"

// When we went to sleep
STATIC int64_t sleepBeganMs = 0;

// Initialize the app
void appInit()
{

    // Create the main task
    xTaskCreate(mainTask, TASKNAME_MAIN, STACKWORDS(TASKSTACK_MAIN), NULL, TASKPRI_MAIN, NULL);

}

// Delay
void appSleep(uint32_t ms)
{
    timerMsSleep(ms);
}

// ISR for interrupts to be processed by the app
void appISR(uint16_t GPIO_Pin)
{

    // GPIO processing
    if (config.gpio) {
        bool attn = false;
#if defined(AUX1_Pin)
        if ((GPIO_Pin & AUX1_Pin) != 0) {
            gstateAuxInterrupt(0);
            attn = true;
        }
#endif
#if defined(AUX2_Pin)
        if ((GPIO_Pin & AUX2_Pin) != 0) {
            gstateAuxInterrupt(1);
            attn = true;
        }
#endif
#if defined(AUX3_Pin)
        if ((GPIO_Pin & AUX3_Pin) != 0) {
            gstateAuxInterrupt(2);
            attn = true;
        }
#endif
#if defined(AUX4_Pin)
        if ((GPIO_Pin & AUX4_Pin) != 0) {
            gstateAuxInterrupt(3);
            attn = true;
        }
#endif
        if (attn) {
            if ((attnMonitoring() & ATTN_AUX_GPIO) != 0) {
                attnSignalFromISR(ATTN_AUX_GPIO);
            }
        }
    }

    // Button processing, noting that the button is ACTIVE LOW
    if (config.neomon) {
#if defined(NEO_BUTTON_Pin)
        if ((GPIO_Pin & NEO_BUTTON_Pin) != 0) {
            buttonPressISR(HAL_GPIO_ReadPin(NEO_BUTTON_GPIO_Port, NEO_BUTTON_Pin) == GPIO_PIN_RESET);
        }
#endif
    }

    // USB Detect processing, noting that the signal is ACTIVE HIGH
#ifdef USB_DETECT_Pin
    if ((GPIO_Pin & USB_DETECT_Pin) != 0) {
        serialResetUSB();
        if ((attnMonitoring() & ATTN_USB) != 0) {
            attnSignalFromISR(ATTN_USB);
        }
        taskGiveAllFromISR();
    }
#endif

}

// Heartbeat for the app
void appHeartbeat()
{
}

// See FreeRTOSConfig.h where this is registered via configPRE_SLEEP_PROCESSING()
// Called by the kernel before it places the MCU into a sleep mode because
void appPreSleepProcessing(uint32_t *ulExpectedIdleTime)
{

    // NOTE:  Additional actions can be taken here to get the power consumption
    // even lower.  For example, peripherals can be turned off here, and then back
    // on again in the post sleep processing function.  For maximum power saving
    //  ensure all unused pins are in their lowest power state.

    // (*ulExpectedIdleTime) is set to 0 to indicate that PreSleepProcessing contains
    // its own wait for interrupt or wait for event instruction and so the kernel
    // vPortSuppressTicksAndSleep function does not need to execute the wfi instruction
    *ulExpectedIdleTime = 0;

    // Remember when we went to sleep
    sleepBeganMs = timerMs();

    // Enter to sleep Mode using the HAL function HAL_PWR_EnterSLEEPMode with WFI instruction
    UTIL_PowerDriver.EnterStopMode();

}

// See FreeRTOSConfig.h where this is registered via configPOST_SLEEP_PROCESSING()
// Called by the kernel before it places the MCU into a sleep mode because
void appPostSleepProcessing(uint32_t *ulExpectedIdleTime)
{

    (void) ulExpectedIdleTime;

    // Tell FreeRTOS how long we were asleep.  Note that we don't
    // ever steptick by 1ms because that's what we see when there is
    // no actual sleep performed, which has the net effect of clocking
    // the timer faster than it's actually supposed to go.
    int64_t elapsedMs = timerMs() - sleepBeganMs;
    if (sleepBeganMs != 0 && elapsedMs > 1) {
        sleepBeganMs = 0;
        vTaskStepTick(pdMS_TO_TICKS(elapsedMs-1));
    }

    // Exit stop mode
    UTIL_PowerDriver.ExitStopMode();

}
