
#include <math.h>
#include "timer_if.h"
#include "main.h"
#include "stm32l4xx_ll_rtc.h"
#include "global.h"
#include "rtc.h"

// Timer driver callbacks handler
const UTIL_TIMER_Driver_s UTIL_TimerDriver = {
    TIMER_IF_Init,
    NULL,

    TIMER_IF_StartTimer,
    TIMER_IF_StopTimer,

    TIMER_IF_SetTimerContext,
    TIMER_IF_GetTimerContext,

    TIMER_IF_GetTimerElapsedTime,
    TIMER_IF_GetTimerValue,
    TIMER_IF_GetMinimumTimeout,

    TIMER_IF_Convert_ms2Tick,
    TIMER_IF_Convert_Tick2ms,
};

// Minimum timeout delay of Alarm in ticks
#define MIN_ALARM_DELAY    3

// Post the RTC log string format to the circular queue for printing in using the polling mode
#ifdef RTIF_DEBUG
#define TIMER_IF_DBG_PRINTF(...) debugf(__VA_ARGS__)
#else
#define TIMER_IF_DBG_PRINTF(...)
#endif

// Indicates if the RTC is already Initialized or not
bool rtcInitialized = false;

// RtcTimerContext
uint32_t RtcTimerContext = 0;

// Init
UTIL_TIMER_Status_t TIMER_IF_Init(void)
{
    UTIL_TIMER_Status_t ret = UTIL_TIMER_OK;

    if (!rtcInitialized) {

        // Init RTC
        MX_RTC_Init();

        // Stop Timer
        TIMER_IF_StopTimer();

        // DeActivate the Alarm A enabled by STM32CubeMX during MX_RTC_Init()
        HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_A);

        // Set the initial context
        TIMER_IF_SetTimerContext();

        // Done with init
        rtcInitialized = true;

    }

    return ret;
}

// Start a timer
UTIL_TIMER_Status_t TIMER_IF_StartTimer(uint32_t timeout)
{
    UTIL_TIMER_Status_t ret = UTIL_TIMER_OK;

    // Stop timer if one is already started
    TIMER_IF_StopTimer();

    // Extend timeout beyond the tick when the timer context was initiated
    timeout += RtcTimerContext;

    // Trace
    TIMER_IF_DBG_PRINTF("Start timer: tick=%d, alarm=%d\n\r",  HAL_GetTick(), timeout);

    // Start timer
    RTC_AlarmTypeDef sAlarm = {0};
    sAlarm.AlarmTime.SubSeconds = UINT32_MAX - timeout;
    sAlarm.AlarmMask = RTC_ALARMMASK_NONE;
    sAlarm.Alarm = RTC_ALARM_A;
    if (HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BCD) != HAL_OK) {
        Error_Handler();
    }

    return ret;
}

// Stop a timer
UTIL_TIMER_Status_t TIMER_IF_StopTimer(void)
{
    UTIL_TIMER_Status_t ret = UTIL_TIMER_OK;
    // Clear RTC Alarm Flag
    __HAL_RTC_ALARM_CLEAR_FLAG(&hrtc, RTC_FLAG_ALRAF);
    // Disable the Alarm A interrupt
    HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_A);
    return ret;
}

// Set the context
uint32_t TIMER_IF_SetTimerContext(void)
{
    RtcTimerContext = HAL_GetTick();
    TIMER_IF_DBG_PRINTF("TIMER_IF_SetTimerContext=%d\n\r", RtcTimerContext);
    return RtcTimerContext;
}

// Get context
uint32_t TIMER_IF_GetTimerContext(void)
{
    TIMER_IF_DBG_PRINTF("TIMER_IF_GetTimerContext=%d\n\r", RtcTimerContext);
    return RtcTimerContext;
}

// Get elapsed time since context was set
uint32_t TIMER_IF_GetTimerElapsedTime(void)
{
    uint32_t ret = 0;
    ret = ((uint32_t)(HAL_GetTick() - RtcTimerContext));
    return ret;
}

// Get ticks value
uint32_t TIMER_IF_GetTimerValue(void)
{
    return HAL_GetTick();
}

// Get min t/o
uint32_t TIMER_IF_GetMinimumTimeout(void)
{
    uint32_t ret = 0;
    ret = (MIN_ALARM_DELAY);
    return ret;
}

// Convert ms to ticks
uint32_t TIMER_IF_Convert_ms2Tick(uint32_t timeMilliSec)
{
    uint32_t ret = 0;
    ret = ((uint32_t)((((uint64_t) timeMilliSec) << RTC_N_PREDIV_S) / 1000));
    return ret;
}

// Convert ticks to ms
uint32_t TIMER_IF_Convert_Tick2ms(uint32_t tick)
{
    uint32_t ret = 0;
    ret = ((uint32_t)((((uint64_t)(tick)) * 1000) >> RTC_N_PREDIV_S));
    return ret;
}

// Delay
void TIMER_IF_DelayMs(uint32_t delayMs)
{
    HAL_Delay(delayMs);
}

// Alarm callback from HAL
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
    UTIL_TIMER_IRQ_Handler();
}

// Set the number of ticks that should be added to the tick count
// because they were lost by a SetTime
uint32_t msToOffset = 0;
void MX_AddTicksToOffset(uint32_t offsetMs)
{
    msToOffset += offsetMs;
}

// Get current time
uint32_t TIMER_IF_GetTime(uint16_t *mSeconds)
{
    int64_t timeMs = MX_RTC_GetMs();
    if (mSeconds != NULL) {
        *mSeconds = (uint16_t) (timeMs % 1000);
    }
    return (uint32_t) (timeMs / 1000);
}
