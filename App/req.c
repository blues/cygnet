
#include "app.h"
#include "request.h"

// Forwards
bool isResetKey(const char *key);

// Process a request.  Note, it is guaranteed that reqJSON[reqJSONLen] == '\0'
err_t reqProcess(bool debugPort, uint8_t *reqJSON, uint32_t reqJSONLen, bool diagAllowed, uint8_t **rspJSON, uint32_t *rspJSONLen)
{
    err_t err = errNone;

    // Mem debugging
#ifdef DEBUG_MEM
    long beginAO = (long) memAllocatedObjects();
    long beginFR = (long) memCurrentlyFree();
    if (!debugPort) {
        debugf("req: mem allocated count:%ld free:%ld\n", (long) memAllocatedObjects(), (long) memCurrentlyFree());
    }
#endif

    // Preset return values in case of failure or 'cmd'
    if (rspJSON != NULL) {
        *rspJSON = NULL;
        *rspJSONLen = 0;
    }

    // Process diagnostic commands, taking advantage of the fact that it is null-terminated
    if (reqJSON[0] != '{') {
        if (!diagAllowed) {
            return errF("+CME ERROR: not a modem");
        }
        err_t err = diagProcess((char *)reqJSON);
        if (err) {
            if (!debugPort) {
                debugf("%s\n", errString(err));
            }
        }
        return errNone;
    }

    // Parse the request
    J *req = JParse((char *)reqJSON);
    if (req == NULL) {
        // If this is a 'cmd', we need to suppress error messages
        // sent back to the host else we'll potentially get out of sync.
        if (strstr((char *)reqJSON, "\"cmd\":") != NULL) {
            return errNone;
        }
        return errF("JSON object expected " ERR_IO);
    }
    bool noReplyRequired = false;
    char *reqtype = JGetString(req, "req");
    if (reqtype[0] == '\0') {
        reqtype = JGetString(req, "cmd");
        if (reqtype[0] == '\0') {
            JDelete(req);
            return errF("no request type specified");
        }
        noReplyRequired = true;
    }
    J *rsp = JCreateObject();
    if (rsp == NULL) {
        JDelete(req);
        return errF("can't alloc response");
    }

    // Copy ID field from request to response
    uint32_t reqid = (uint32_t) JGetNumber(req, "id");
    if (reqid != 0) {
        JAddNumberToObject(rsp, "id", reqid);
    }

    // Don't allow debug output that could interfere with JSON requests
    bool debugWasEnabled = MX_DBG_Enable(false);
    if (JGetBool(req, "verbose")) {
        MX_DBG_Enable(debugWasEnabled);
    }

    // Debug
    if (!debugPort) {
        MX_DBG_Enable(debugWasEnabled);
        debugMessage(">> ");
        debugMessage((char *)reqJSON);
        debugMessage("\n");
    }

    // Process requests
    for (;;) {

        if (strEQL(reqtype, ReqEcho)) {
            J *temp = rsp;
            rsp = req;
            req = temp;
            break;
        }

        // Unrecognized request
        err = errF("unrecognized request: %s", reqtype);
        break;
    }

    // Restore debug output
    if (!debugPort) {
        MX_DBG_Enable(debugWasEnabled);
    }

    // If no reply required, we're done
    JDelete(req);
    if (noReplyRequired) {
        JDelete(rsp);
#ifdef DEBUG_MEM
        if (!debugPort) {
            debugf("req: mem allocated count:%ld (+%ld) free:%ld (+%ld)\n", (long) memAllocatedObjects(), (long)memAllocatedObjects()-beginAO, (long) memCurrentlyFree(), (long)memCurrentlyFree()-beginFR);
        }
#endif
        return errNone;
    }

    // Handle errors
    if (err) {
        JDelete(rsp);
        return err;
    }

    // Return successful response
    if (rspJSON == NULL) {
        JDelete(rsp);
    } else {
        char *json = JPrintUnformattedOmitEmpty(rsp);
        JDelete(rsp);
        *rspJSON = (uint8_t *) json;
        *rspJSONLen = strlen(json);
        if (!debugPort) {
            debugMessage("<< ");
            debugMessage(json);
            debugMessage("\n");
        }
    }
#ifdef DEBUG_MEM
    if (!debugPort) {
        debugf("req: mem allocated count:%ld (+%ld) free:%ld (+%ld)\n", (long) memAllocatedObjects(), (long)memAllocatedObjects()-beginAO, (long) memCurrentlyFree(), (long)memCurrentlyFree()-beginFR);
    }
#endif
    return errNone;

}

// Check for reset key, including old versions of same
bool isResetKey(const char *key)
{
    if (strEQL(key, "-")) {
        return true;
    }
    if (strEQL(key, "default")) {
        return true;
    }
    if (strEQL(key, "reset")) {
        return true;
    }
    return false;
}
