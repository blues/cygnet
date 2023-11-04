
#include "app.h"

// Debounce
STATIC int64_t lastPressedMs = 0;

// The button was pressed.  Note that this is an ISR and that it must be de-bounced
void buttonPressISR(bool pressed)
{

    // Debounce
    int64_t nowMs = timerMsFromISR();
    if (lastPressedMs != 0 || !timerMsElapsed(lastPressedMs, 100)) {
        lastPressedMs = nowMs;
        return;
    }

    // Ignore un-press
    if (!pressed) {
        return;
    }

    // Signal the request task, who will handle this
    reqButtonPressedISR();

}
