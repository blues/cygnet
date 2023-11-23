
#include "main.h"
#include "stm32_lpm.h"
#include "stm32_lpm_if.h"
#include "usart.h"
#include "rng.h"
#include "dma.h"
#include "gpio.h"

// Stop/Resume data
bool wasStopped = false;
uint32_t peripheralsToResume;

// Power driver callbacks handler
const struct UTIL_LPM_Driver_s UTIL_PowerDriver = {
    PWR_EnterSleepMode,
    PWR_ExitSleepMode,
    PWR_EnterStopMode,
    PWR_ExitStopMode,
    PWR_EnterOffMode,
    PWR_ExitOffMode,
};

// Pause
void PWR_EnterOffMode(void)
{
}

// Resume
void PWR_ExitOffMode(void)
{
}

// Enter sleep
void PWR_EnterSleepMode(void)
{
    HAL_SuspendTick();
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
}

// Exit sleep
void PWR_ExitSleepMode(void)
{
    HAL_ResumeTick();
}

// Stop
void PWR_EnterStopMode(void)
{

    // Peripherals we can handle
    uint32_t weLeaveOn = PERIPHERAL_LPUART1;
    uint32_t weCanPowerOff = PERIPHERAL_RNG | PERIPHERAL_USART1 | PERIPHERAL_USART2;
    uint32_t wouldKeepUsAwake = peripherals & ~(weLeaveOn | weCanPowerOff);

    // Other things that keep us awake
    bool stayAwake = false;
    if (MX_DBG_Active()) {
        stayAwake = true;
    }
    if (!appSleepAllowed()) {
        stayAwake = true;
    }
#ifdef USB_DETECT_Pin
    if (HAL_GPIO_ReadPin(USB_DETECT_GPIO_Port, USB_DETECT_Pin) == GPIO_PIN_SET) {
        stayAwake = true;
    }
#else
    // We can't know what the right strategy is if we can't sense USB
    stayAwake = true;
#endif

    // If any peripherals are left initialized other than USART1, don't stop
    if (wouldKeepUsAwake != 0 || stayAwake) {
        wasStopped = false;
        return;
    }
    peripheralsToResume = peripherals;

    // Suspend peripherals
    if (peripherals & PERIPHERAL_USART2) {
        MX_USART2_UART_DeInit();
    }
    if (peripherals & PERIPHERAL_USART1) {
        MX_USART1_UART_DeInit();
    }
    if (peripherals & PERIPHERAL_RNG) {
        MX_RNG_DeInit();
    }

    // Disable DMA
    MX_DMA_DeInit();

    // Suspend sysTick
    HAL_SuspendTick();

    // Prepare 'awake' peripherals so they wake-up from STOP
    MX_LPUART1_UART_Suspend();

    // As a defensive measure, make sure all the peripherals are OFF that are supposed to be off
    MX_GPIO_Stop();

    // Trace
#ifdef DEBUG_BUSY_STOP
    HAL_GPIO_WritePin(LED_BUSY_GPIO_Port, LED_BUSY_Pin, GPIO_PIN_RESET);
#endif

    // Clear Status Flag before entering STOP/STANDBY Mode
    wasStopped = true;
    HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);

}

// Resume
void PWR_ExitStopMode(void)
{

    // Exit if wasn't stopped
    if (!wasStopped) {
        return;
    }

    // Trace
#ifdef DEBUG_BUSY_STOP
    HAL_GPIO_WritePin(LED_BUSY_GPIO_Port, LED_BUSY_Pin, GPIO_PIN_SET);
#endif

    // Resume peripherals that were awake
    MX_LPUART1_UART_Resume();

    // Resume DMA
    MX_DMA_Init();

    // Resume sysTick : work around for debugger problem in dual core
    HAL_ResumeTick();

    // Resume peripherals that weren't retained but which are active
    if ((peripheralsToResume & PERIPHERAL_USART1) != 0) {
        MX_USART1_UART_ReInit();
    }
    if ((peripheralsToResume & PERIPHERAL_USART2) != 0) {
        MX_USART2_UART_ReInit();
    }

}
