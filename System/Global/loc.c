// Copyright 2017 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "app.h"
#include "global.h"

// Is location set/valid
STATIC bool locationValid = false;
STATIC double locationLat = 0;
STATIC double locationLon = 0;
STATIC uint32_t locationSecs = 0;

// Set the location
bool locSet(double lat, double lon)
{
    locationLat = lat;
    locationLon = lon;
    if (timeIsValid()) {
        locationSecs = timeSecs();
    } else {
        locationSecs = 0;
    }
    locationValid = true;
    return true;
}

// Get the location
bool locGet(double *lat, double *lon, uint32_t *when)
{
    if (!locationValid) {
        if (lat != NULL) {
            *lat = 0.0;
        }
        if (lon != NULL) {
            *lon = 0.0;
        }
        if (when != NULL) {
            *when = 0;
        }
        return false;
    }
    if (lat != NULL) {
        *lat = locationLat;
    }
    if (lon != NULL) {
        *lon = locationLon;
    }
    if (when != NULL) {
        *when = locationSecs;
    }
    return true;
}

// See if the location is valid
bool locValid(void)
{
    return locationValid;
}

// Invalidate the current location
void locInvalidate(void)
{
    locationValid = false;
}
