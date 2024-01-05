
#include "app.h"
#include "main.h"

// Signal on the module's LED that we have restated
void ledRestartSignal()
{
    for (int i=10; i>-10; i-=1) {
        HAL_GPIO_WritePin(LED_BUSY_GPIO_Port, LED_BUSY_Pin, GPIO_PIN_SET);
        for (volatile int j=0; j<GMAX(i,0)+4; j++) for (volatile int k=0; k<40000; k++) ;
        HAL_GPIO_WritePin(LED_BUSY_GPIO_Port, LED_BUSY_Pin, GPIO_PIN_RESET);
        for (volatile int j=0; j<GMAX(i,0)+4; j++) for (volatile int k=0; k<30000; k++) ;
    }
#ifdef DEBUG_BUSY
    HAL_GPIO_WritePin(LED_BUSY_GPIO_Port, LED_BUSY_Pin, GPIO_PIN_SET);
#endif
}

// Signal BUSY on or off
void ledBusy(bool enable)
{
#ifndef DEBUG_BUSY
    HAL_GPIO_WritePin(LED_BUSY_GPIO_Port, LED_BUSY_Pin, enable ? GPIO_PIN_SET : GPIO_PIN_RESET);
#endif
}
