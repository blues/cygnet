// Copyright 2021 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include <stdarg.h>
#include "board.h"
#include "app.h"

// Structure to define each GPIO by name, port, and pin
typedef struct {
    char *name;
    GPIO_TypeDef *port;
    uint16_t pin;
    IRQn_Type irqn;
} gpiodef;

// Enumerated index for the GPIOs defined in the gpiodef table below.
enum
{
  GPIO_USB_DETECT,
  GPIO_A0,
  GPIO_A1,
  GPIO_A2,
  GPIO_A3,
  GPIO_A4,
  GPIO_A5,
  GPIO_D5,
  GPIO_D6,
  GPIO_D9,
  GPIO_D10,
  GPIO_D11,
  GPIO_D12,
  GPIO_D13,
  GPIO_LAST,
};

// Port flags
// GPIO table, which MUST be ordered exactly as defined in the above enum.
STATIC gpiodef gpio[GPIO_LAST+1] = {
    {"USB_DETECT", USB_DETECT_GPIO_Port, USB_DETECT_Pin, USB_DETECT_IRQn},  
    {"A0", A0_GPIO_Port, A0_Pin, A0_IRQn},
    {"A1", A1_GPIO_Port, A1_Pin, A1_IRQn},
    {"A2", A2_GPIO_Port, A2_Pin, A2_IRQn},
    {"A3", A3_GPIO_Port, A3_Pin, A3_IRQn},
    {"A4", A4_GPIO_Port, A4_Pin, A4_IRQn},
    {"A5", A5_GPIO_Port, A5_Pin, A5_IRQn},
    {"D5", D5_GPIO_Port, D5_Pin, D5_IRQn},
    {"D6", D6_GPIO_Port, D6_Pin, D6_IRQn},
    {"D9", D9_GPIO_Port, D9_Pin, D9_IRQn},
    {"D10", D10_GPIO_Port, D10_Pin, D10_IRQn},
    {"D11", D11_GPIO_Port, D11_Pin, D11_IRQn},
    {"D12", D12_GPIO_Port, D12_Pin, D12_IRQn},
    {"D13", D13_GPIO_Port, D13_Pin, D13_IRQn},      
    {"", 0, 0, NULL},
};

// Do port and pin mapping based on board version
void gpioMap(int id, GPIO_TypeDef **port, uint16_t *pin, IRQn_Type *irqn)
{
    *port = gpio[id].port;
    *pin = gpio[id].pin;
    *irqn = gpio[id].irqn;
}

// The HAL is supposed to be linked WITHOUT the app.  However, for reasons of leveraging the actual code
// that will talk with the device - much of which exists inside the app itself - we have several symbols
// here that control whether or not we're linking with the app.  Just turn them OFF for a minimal HAL build.
#define TEST_GPIO       true

// Continuity tests, assuming test fixture wiring
typedef struct {
    char *name;
    int gpio;
    bool  pullup;
} pinDef;

typedef struct {
    pinDef a;
    pinDef b;
} pinPair;

STATIC pinPair notecard_pins[] = {
    {
        .a={ .name="A0", .gpio=GPIO_A0 },
        .b={ .name="A1", .gpio=GPIO_A1 }
    },
    {
        .a={ .name="A2", .gpio=GPIO_A2 },
        .b={ .name="A3", .gpio=GPIO_A3 }
    },
    {
        .a={ .name="A4", .gpio=GPIO_A4 },
        .b={ .name="A5", .gpio=GPIO_A5 }
    },
    {
        .a={ .name="D5", .gpio=GPIO_D5 },
        .b={ .name="D6", .gpio=GPIO_D6}
    },
/*    {
        // Since we have an odd number of GPIO, this one gets skipped.
        .b={ .name="D9", .gpio=8 }
    },*/
    {
        .a={ .name="D10", .gpio=GPIO_D10 },
        .b={ .name="D11", .gpio=GPIO_D11 }
    },
    {
        .a={ .name="D12", .gpio=GPIO_D12 },
        .b={ .name="D13", .gpio=GPIO_D13 }
    },
    {
        .a={ .name="" }
    },
};

// Build string
char * osBuildString()
{
    return PRODUCT_BUILD;
}

// Forwards
#define EOL "\r\n"
void textOutN(char *text, uint32_t len);
void textOut(const char *format, ...);

// USB
char printbuf[256];
uint8_t usbrb[128] = {0};
volatile uint16_t usbrbReceived = 0;

// Abort handling
#define failReasonOrAbortMsg() (failReason != NULL ? failReason : failMsg(NULL));
#define failMsg(x) (postAbort ? "*** aborted ***" : (x))
volatile bool postAbort = false;

char FailTest[128];
char FailReason[128];

// Forwards
bool continuityExists(char *nameA, GPIO_TypeDef *portA, uint16_t pinA, bool pullupA, char *nameB, GPIO_TypeDef *portB, uint16_t pinB, bool pullupB, char *result, uint32_t resultLen);

// POWER-ON SELF TEST procedure returns FALSE if a known failure, or TRUE if the module may or may not be OK.
bool postSelfTest()
{
    char *failReason = NULL;
    GPIO_TypeDef *port, *a_port, *b_port;
    uint16_t pin, a_pin, b_pin;
    IRQn_Type irqn, a_irqn, b_irqn;

    // Begin the test
    textOut(EOL EOL "================================" EOL);
    textOut("=== BEGIN POWER-ON SELF TEST ===" EOL);
    textOut("================================" EOL EOL);
    textOut(osBuildString());
    textOut(EOL EOL);

    // Do external pad continuity tests if we're in a test jig.  If it fails, do it again just in case there is a situation in
    // which FreeRTOS had had an I/O outstanding and its ISR had messed with the I/O during our continuity test
#if TEST_GPIO
    if (failReason == NULL) {
        textOut("Testing GPIOs" EOL);
        pinPair *pins = notecard_pins;
        // Test external pads
        bool firstContinuityTestFailed = false;;
        for (int i=0; pins[i].a.name[0] != '\0'; i++) {
            gpioMap(pins[i].a.gpio, &a_port, &a_pin, &a_irqn);
            gpioMap(pins[i].b.gpio, &b_port, &b_pin, &b_irqn);
            if (!continuityExists(pins[i].a.name, a_port, a_pin, pins[i].a.pullup, pins[i].b.name, b_port, b_pin, pins[i].b.pullup, NULL, 0)) {
                firstContinuityTestFailed = true;
                break;
            }
        }
        // Test external pads a second time if failure
        if (firstContinuityTestFailed) {
            for (int i=0; pins[i].a.name[0] != '\0'; i++) {
                gpioMap(pins[i].a.gpio, &a_port, &a_pin, &a_irqn);
                gpioMap(pins[i].b.gpio, &b_port, &b_pin, &b_irqn);
                if (!continuityExists(pins[i].a.name, a_port, a_pin, pins[i].a.pullup, pins[i].b.name, b_port, b_pin, pins[i].b.pullup, FailReason, sizeof(FailReason))) {
                    snprintf(FailTest, sizeof(FailTest), "gpio");
                    failReason = FailReason;
                    textOut("gpio FAIL (%s)" EOL, failReason);
                    break;
                }
            }
        }
        // Test misc pins for obvious failures
        GPIO_InitTypeDef init = {0};
        if (failReason == NULL) {
            // Since we're doing this test via USB, it had better be high
            memset(&init, 0, sizeof(init));
            gpioMap(GPIO_USB_DETECT, &port, &pin, &irqn);
            init.Mode = GPIO_MODE_INPUT;
            init.Pull = GPIO_NOPULL;
            init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
            init.Pin = pin;
            HAL_GPIO_Init(port, &init);
            HAL_Delay(1);
            if (HAL_GPIO_ReadPin(port, pin) != GPIO_PIN_SET) {
                strLcpy(FailTest, "gpio pin USB_DETECT");
                failReason = "pin isn't high";
            }
        }
        // Done
        if (failReason != NULL) {
            textOut("gpio FAIL (%s %s)" EOL, FailTest, failReason);
        } else {
            textOut("gpio PASS" EOL);
        }
    }
#endif // TEST_GPIO

    // Results
    textOut(EOL "=================================" EOL);
    if (postAbort) {
        textOut("======= POWER-ON SELF TEST ABORTED" EOL);
    } else if (failReason != NULL) {
        textOut("======= POWER-ON SELF TEST FAILED: %s: %s" EOL, FailTest, FailReason);
    } else {
        textOut("=== POWER-ON SELF TEST PASSED" EOL);
    }
    textOut("=================================" EOL EOL);

    return !((postAbort) || (failReason != NULL));
}

// Counted output handler
void textOutN(char *text, uint32_t len)
{
    debugf(text);
}

// Output handler
void textOut(const char *format, ...)
{
    uint32_t printbufLeft = sizeof(printbuf);

    // Process the printf portion of the work to be done
    va_list args;
    va_start (args, format);
    vsnprintf (printbuf, printbufLeft, format, args);
    va_end (args);

    // Make sure it ends in a newline if it's truncated,
    // just for aesthetics
    strlcpy(&printbuf[printbufLeft-(strlen(EOL)+1)], EOL, sizeof(printbuf));

    // Output it
    textOutN(printbuf, strlen(printbuf));
}

// Test for continuity between two GPIO pins, leaving the pins in an "analog input" state.  In this test, we first do
// bidirectional testing of whether one side or the other of a cross-wired pair of pins can sense a "strong" output,
// and second we do bidirectional testing of whether one side or the other can sense a software pull-up asserted by the other.
bool continuityExists(char *nameA, GPIO_TypeDef *portA, uint16_t pinA, bool pullupA, char *nameB, GPIO_TypeDef *portB, uint16_t pinB, bool pullupB, char *result, uint32_t resultLen)
{
    bool pullup = (pullupA || pullupB);
    uint16_t pinOutput, pinInput;
    GPIO_TypeDef *portOutput, *portInput;
    GPIO_InitTypeDef init = {0};
    char *failPin;
    char *failFrom = NULL;
    char *failReason = NULL;

    // Known state for scope
    memset(&init, 0, sizeof(init));
    init.Mode = GPIO_MODE_ANALOG;
    init.Pull = GPIO_NOPULL;
    init.Pin = pinA;
    HAL_GPIO_Init(portA, &init);
    init.Pin = pinB;
    HAL_GPIO_Init(portB, &init);
    HAL_Delay(5);

    // Verify that toggling A is visible on B
    if (failReason == NULL) {
        failPin = nameB;
        failFrom = nameA;
        portInput = portB;
        pinInput = pinB;
        portOutput = portA;
        pinOutput = pinA;
        memset(&init, 0, sizeof(init));
        init.Mode = GPIO_MODE_OUTPUT_PP;
        init.Pin = pinOutput;
        HAL_GPIO_Init(portOutput, &init);
        HAL_GPIO_WritePin(portOutput, pinOutput, GPIO_PIN_RESET);
        HAL_Delay(1);
        memset(&init, 0, sizeof(init));
        init.Mode = GPIO_MODE_INPUT;
        init.Pull = GPIO_PULLUP;
        init.Pin = pinInput;
        init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        HAL_GPIO_Init(portInput, &init);
        HAL_Delay(1);
        if (HAL_GPIO_ReadPin(portInput, pinInput) != GPIO_PIN_RESET) {
            failReason = "doesn't read a low";
        } else {
            memset(&init, 0, sizeof(init));
            init.Mode = GPIO_MODE_INPUT;
            init.Pull = pullup ? GPIO_NOPULL : GPIO_PULLDOWN;
            init.Pin = pinInput;
            init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
            HAL_GPIO_Init(portInput, &init);
            HAL_Delay(1);
            HAL_GPIO_WritePin(portOutput, pinOutput, GPIO_PIN_SET);
            HAL_Delay(1);
            if (HAL_GPIO_ReadPin(portInput, pinInput) != GPIO_PIN_SET) {
                failReason = "doesn't read a high";
            }
        }
    }

    // Known state for scope
    memset(&init, 0, sizeof(init));
    init.Mode = GPIO_MODE_ANALOG;
    init.Pull = GPIO_NOPULL;
    init.Pin = pinA;
    HAL_GPIO_Init(portA, &init);
    init.Pin = pinB;
    HAL_GPIO_Init(portB, &init);
    HAL_Delay(5);

    // Verify that toggling B is visible on A
    if (failReason == NULL) {
        failPin = nameA;
        failFrom = nameB;
        portInput = portA;
        pinInput = pinA;
        portOutput = portB;
        pinOutput = pinB;
        memset(&init, 0, sizeof(init));
        init.Mode = GPIO_MODE_OUTPUT_PP;
        init.Pin = pinOutput;
        HAL_GPIO_Init(portOutput, &init);
        HAL_GPIO_WritePin(portOutput, pinOutput, GPIO_PIN_RESET);
        HAL_Delay(1);
        memset(&init, 0, sizeof(init));
        init.Mode = GPIO_MODE_INPUT;
        init.Pull = GPIO_PULLUP;
        init.Pin = pinInput;
        init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        HAL_GPIO_Init(portInput, &init);
        HAL_Delay(1);
        if (HAL_GPIO_ReadPin(portInput, pinInput) != GPIO_PIN_RESET) {
            failReason = "doesn't read a low";
        } else {
            memset(&init, 0, sizeof(init));
            init.Mode = GPIO_MODE_INPUT;
            init.Pull = pullup ? GPIO_NOPULL : GPIO_PULLDOWN;
            init.Pin = pinInput;
            init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
            HAL_GPIO_Init(portInput, &init);
            HAL_Delay(1);
            HAL_GPIO_WritePin(portOutput, pinOutput, GPIO_PIN_SET);
            HAL_Delay(1);
            if (HAL_GPIO_ReadPin(portInput, pinInput) != GPIO_PIN_SET) {
                failReason = "doesn't read a high";
            }
        }
    }

    // Known state for scope
    memset(&init, 0, sizeof(init));
    init.Mode = GPIO_MODE_ANALOG;
    init.Pull = GPIO_NOPULL;
    init.Pin = pinA;
    HAL_GPIO_Init(portA, &init);
    init.Pin = pinB;
    HAL_GPIO_Init(portB, &init);
    HAL_Delay(5);

    // Verify that B isn't pulled up or pulled down, assuming it is connected to A
    // If this test succeeds, it means that the pullup/down is strong enough to
    // pull the other side
    if (failReason == NULL) {
        failPin = nameB;
        portInput = portB;
        pinInput = pinB;
        portOutput = portA;
        pinOutput = pinA;
        memset(&init, 0, sizeof(init));
        init.Mode = GPIO_MODE_INPUT;
        init.Pull = pullup ? GPIO_NOPULL : GPIO_PULLDOWN;
        init.Pin = pinInput;
        init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        HAL_GPIO_Init(portInput, &init);
        HAL_Delay(1);
        memset(&init, 0, sizeof(init));
        init.Mode = GPIO_MODE_INPUT;
        init.Pull = GPIO_NOPULL;
        init.Pin = pinOutput;
        HAL_GPIO_Init(portOutput, &init);
        HAL_Delay(1);
        if (!pullup && HAL_GPIO_ReadPin(portOutput, pinOutput) != GPIO_PIN_RESET) {
            failReason = "pulled high";
        } else {
            memset(&init, 0, sizeof(init));
            init.Mode = GPIO_MODE_INPUT;
            init.Pull = GPIO_PULLUP;
            init.Pin = pinInput;
            init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
            HAL_GPIO_Init(portInput, &init);
            HAL_Delay(1);
            if (HAL_GPIO_ReadPin(portOutput, pinOutput) != GPIO_PIN_SET) {
                failReason = "pulled low";
            }
        }
    }

    // Known state for scope
    memset(&init, 0, sizeof(init));
    init.Mode = GPIO_MODE_ANALOG;
    init.Pull = GPIO_NOPULL;
    init.Pin = pinA;
    HAL_GPIO_Init(portA, &init);
    init.Pin = pinB;
    HAL_GPIO_Init(portB, &init);
    HAL_Delay(5);

    // Verify that A isn't pulled up or pulled down, assuming it is connected to B
    // If this test succeeds, it means that the pullup/down is strong enough to
    // pull the other side
    if (failReason == NULL) {
        failPin = nameA;
        portInput = portA;
        pinInput = pinA;
        portOutput = portB;
        pinOutput = pinB;
        memset(&init, 0, sizeof(init));
        init.Mode = GPIO_MODE_INPUT;
        init.Pull = pullup ? GPIO_NOPULL : GPIO_PULLDOWN;
        init.Pin = pinInput;
        init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        HAL_GPIO_Init(portInput, &init);
        HAL_Delay(1);
        memset(&init, 0, sizeof(init));
        init.Mode = GPIO_MODE_INPUT;
        init.Pull = GPIO_NOPULL;
        init.Pin = pinOutput;
        HAL_GPIO_Init(portOutput, &init);
        HAL_Delay(1);
        if (!pullup && HAL_GPIO_ReadPin(portOutput, pinOutput) != GPIO_PIN_RESET) {
            failReason = "pulled high";
        } else {
            memset(&init, 0, sizeof(init));
            init.Mode = GPIO_MODE_INPUT;
            init.Pull = GPIO_PULLUP;
            init.Pin = pinInput;
            init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
            HAL_GPIO_Init(portInput, &init);
            HAL_Delay(1);
            if (HAL_GPIO_ReadPin(portOutput, pinOutput) != GPIO_PIN_SET) {
                failReason = "pulled low";
            }
        }
    }

    // Known state for scope
    memset(&init, 0, sizeof(init));
    init.Mode = GPIO_MODE_ANALOG;
    init.Pull = GPIO_NOPULL;
    init.Pin = pinA;
    HAL_GPIO_Init(portA, &init);
    init.Pin = pinB;
    HAL_GPIO_Init(portB, &init);
    HAL_Delay(10);

    // Process success or failure
    if (failReason == NULL) {
        return true;
    }

    // Store the reason
    if (result != NULL) {
        if (failFrom == NULL) {
            snprintf(result, resultLen, "%s pin %s", failReason, failPin);
        } else {
            snprintf(result, resultLen, "%s pin %s from pin %s", failReason, failPin, failFrom);
        }
    }

    return false;

}
