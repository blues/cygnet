// Copyright 2024 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "app.h"

// Process a request.  Note, it is guaranteed that reqJSON[reqJSONLen] == '\0'
err_t reqProcess(bool debugPort, uint8_t *reqJSON, bool diagAllowed)
{
    err_t err = errNone;

    // Process diagnostic commands
    if (reqJSON[0] != '{') {
        if (!diagAllowed) {
            return errF("diagnostics not allowed on this port");
        }
        err_t err = diagProcess((char *)reqJSON);
        if (err) {
            if (!debugPort) {
                debugf("%s\n", errString(err));
            }
        }
        return errNone;
    }

    // An example of where an app might process JSON requests
    err = errF("JSON requests not implemented");

    // Done
    return err;

}
