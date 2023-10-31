
#include "app.h"
#include "request.h"

// Forwards
bool isResetKey(const char *key);

// Process a request.  Note, it is guaranteed that reqJSON[reqJSONLen] == '\0'
err_t reqProcess(bool debugPort, uint8_t *reqJSON, uint32_t reqJSONLen, bool diagAllowed, J **retRsp)
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
    if (retRsp != NULL) {
        *retRsp = NULL;
    }

    // Process diagnostic commands, taking advantage of the fact that it is null-terminated
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

    // A "req" implies "request/response".  Otherwise, it is simply a unidirectional
    // message whose type is accepted in either a "cmd" or "type" field.
    bool noReplyRequired = false;
    char *reqtype = JGetString(req, "req");
    if (reqtype[0] == '\0') {
        noReplyRequired = true;
        reqtype = JGetString(req, "cmd");
        if (reqtype[0] == '\0') {
            reqtype = JGetString(req, "type");
        }
        if (reqtype[0] == '\0') {
            JDelete(req);
            return errF("no request type specified");
        }
    }

    // Prepare for the response
    J *rsp = JCreateObject();
    if (rsp == NULL) {
        JDelete(req);
        return errF("can't alloc response");
    }

    // Copy ID field from request to response
    uint32_t reqid = (uint32_t) JGetNumber(req, FieldID);
    if (reqid != 0) {
        JAddNumberToObject(rsp, FieldID, reqid);
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

        if (strEQL(reqtype, MsgHello)) {
            monitorReceivedHello();
            uint32_t time = JGetInt(req, "time");
            if (time != 0) {
                timeSet(time);
            }
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

    // Mem tracing
#ifdef DEBUG_MEM
    if (!debugPort) {
        debugf("req: mem allocated count:%ld (+%ld) free:%ld (+%ld)\n", (long) memAllocatedObjects(), (long)memAllocatedObjects()-beginAO, (long) memCurrentlyFree(), (long)memCurrentlyFree()-beginFR);
    }
#endif

    // Done
    *retRsp = rsp;
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
