// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

// Allow overrides, but by defining them up-front rather than later
#include "app_product.h"

// Product version
#ifndef PRODUCT_MANUFACTURER
#error "In your IDE please define PRODUCT_MANUFACTURER as something like 'Acme Inc'"
#endif
#ifndef PRODUCT_DISPLAY_NAME
#error "In your IDE please define PRODUCT_DISPLAY_NAME as something like "Power Monitor"
#endif
#ifndef PRODUCT_PROGRAMMATIC_NAME
#error "In your IDE please define PRODUCT_PROGRAMMATIC_NAME as something like "powermon"
#endif

// USB overrides in case you'd like to assign your own
#ifndef PRODUCT_USB_VID
#define PRODUCT_USB_VID             12452   // 0x30A4 Inca Roads LLC
#endif
#ifndef PRODUCT_USB_PID
#define PRODUCT_USB_PID             3       // Inca Roads LLC ID is 3 for the Cygnet
#endif

// Version numbers:
//  MAJOR changes means huge features and that there may be incompatibilities
//  MINOR changes means that there may be new features but there are guaranteed to be no regressions
//  RELEASE means just bug fixes and there are guaranteed to be no regressions
//	All MUST BE NUMERIC, and first and last digits must not be 0
#ifndef PRODUCT_MAJOR_VERSION
#error "In your IDE please define PRODUCT_MAJOR_VERSION as a number"
#endif
#ifndef PRODUCT_MINOR_VERSION
#error "In your IDE please define PRODUCT_MINOR_VERSION as a number"
#endif
#ifndef PRODUCT_PATCH_VERSION
#error "In your IDE please define PRODUCT_PATCH_VERSION as a number"
#endif

// Composite version
#define PRODUCT_VERSION STRINGIFY(PRODUCT_MAJOR_VERSION) "." STRINGIFY(PRODUCT_MINOR_VERSION) "." STRINGIFY(PRODUCT_PATCH_VERSION)

// We can search for these strings in the .bin to validate them for DFU.
#define PRODUCT_BUILD PRODUCT_MANUFACTURER " " PRODUCT_DISPLAY_NAME " Firmware " STRINGIFY(PRODUCT_MAJOR_VERSION) "." STRINGIFY(PRODUCT_MINOR_VERSION) "." STRINGIFY(PRODUCT_PATCH_VERSION) " on " __DATE__ " at " __TIME__

#define QUOTE "\""
#define PRODUCT_CONFIG_SIGNATURE "firmware::info:"
#define PRODUCT_CONFIG PRODUCT_CONFIG_SIGNATURE                         \
    "{" QUOTE "org" QUOTE ":" QUOTE PRODUCT_MANUFACTURER QUOTE          \
    "," QUOTE "product" QUOTE ":" QUOTE PRODUCT_DISPLAY_NAME QUOTE      \
    "," QUOTE "ver_major" QUOTE ":" STRINGIFY(PRODUCT_MAJOR_VERSION)    \
    "," QUOTE "ver_minor" QUOTE ":" STRINGIFY(PRODUCT_MINOR_VERSION)    \
    "," QUOTE "ver_patch" QUOTE ":" STRINGIFY(PRODUCT_PATCH_VERSION)    \
    "," QUOTE "built" QUOTE ":" QUOTE  __DATE__ " " __TIME__ QUOTE      \
    "}"

// We must stringify above
#define STRINGIFY(x) STRINGIFY_(x)
#define STRINGIFY_(x) #x
