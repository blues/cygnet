// Copyright 2024 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

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

    // Process button press outside of ISR level
    reqButtonPressedISR();

}
