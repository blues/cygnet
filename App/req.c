
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
    // message whose type is indicated in the "cmd" field
    bool noReplyRequired = false;
    char *reqtype = JGetString(req, "req");
    if (reqtype[0] == '\0') {
        noReplyRequired = true;
        reqtype = JGetString(req, "cmd");
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

    // All requests are capable of accepting updates to location and time
    timeSetIfBetter(JGetInt(req, "time"));
    locSetIfBetter(JGetNumber(req, "lat"), JGetNumber(req, "lon"), JGetInt(req, "ltime"));

    // If any of the config items happen to be present in the body, on any request, set them
    configSet(JGetObjectItem(req, "body"));

    // Process requests
    for (;;) {

        // Echo back a hello request if our time is up-to-date, because we can't continue otherwise
        if (strEQL(reqtype, ReqHello)) {
            configReceivedHello = true;
            break;
        }

        // Request to use the modem
        if (strEQL(reqtype, ReqConnect)) {
            if (!modemWorkExists(workModemConnect)) {
                err = modemEnqueueWork(NTN_CONNECTING, workModemConnect, JDetachItemFromObject(req, "body"), NULL);
            }
            break;
        }

        // Request to disconnect the modem after work has been completed
        if (strEQL(reqtype, ReqDisconnect)) {
            modemRemoveWork(workModemDisconnect);
            err = modemEnqueueWork(NTN_DISCONNECTING, workModemDisconnect, NULL, NULL);
            break;
        }

        // Request to perform an uplink
        if (strEQL(reqtype, ReqUplink)) {
            err = modemEnqueueWork(NTN_UPLINKING, workModemUplink, JDetachItemFromObject(req, "body"), JGetString(req, "payload"));
            break;
        }

        // Request to send status info
        if (strEQL(reqtype, ReqStatus)) {
            err = modemReportStatus();
            break;
        }

        // Request to enable the GPS
        if (strEQL(reqtype, ReqGpsEnable)) {
            err = modemEnqueueWork(NTN_ENABLING_GPS, workEnableGps, NULL, NULL);
            break;
        }

        // Request to disable the GPS
        if (strEQL(reqtype, ReqGpsDisable)) {
            err = modemEnqueueWork(NTN_DISABLING_GPS, workDisableGps, NULL, NULL);
            break;
        }

        // Request to get or set the test certificate
        // If body is specified, this request is from the test fixture to set
        // the test certificate in flash, else it's a request to display it.
        if (strEQL(reqtype, ReqCardTest)) {
            J *body = JGetObjectItem(req, "body");
            if (body != NULL) {
                err = postSelfTest(JGetBool(req, "verify"), body);
                break;
            }
            J *tcert = postGetTestCert();
            if (tcert == NULL) {
                err = errF("no test certificate found");
            } else {
                JAddItemToObject(rsp, "body", tcert);
            }
            break;
        }

        // Restart
        if (strEQL(reqtype, ReqCardRestart)) {
            MX_Restart();
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

    // If we haven't already deleted the request, delete it
    if (req != NULL) {
        JDelete(req);
    }

    // If no reply required, we're done
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
