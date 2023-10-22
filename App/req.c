
#include "notelib.h"
#include "request.h"
#include "main.h"

// Forwards
bool isResetKey(const char *key);

// Process a request.  Note, it is guaranteed that reqJSON[reqJSONLen] == '\0'
err_t reqProcess(bool debugPort, uint8_t *reqJSON, uint32_t reqJSONLen, bool diagAllowed, uint8_t **rspJSON, uint32_t *rspJSONLen)
{
    err_t err = errNone;
    char buf[128];      // General purpose

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
    if (config.traceRequests || configp.traceRequests) {
        if (!debugPort) {
            MX_DBG_Enable(debugWasEnabled);
            debugMessage(">> ");
            debugMessage((char *)reqJSON);
            debugMessage("\n");
        }
    }

    // Process requests
    for (;;) {

        if (strEQL(reqtype, ReqEcho)) {
            J *temp = rsp;
            rsp = req;
            req = temp;
            break;
        }

        if (strEQL(reqtype, ReqCardTest)) {
            char *mode = JGetString(req, "mode");
            if (strEQL(mode, "notemem") || strEQL(mode, "mem")) {
                J *body = JCreateObject();
                JAddNumberToObject(body, "mem_app_heap", heapFreeAtStartup);
                JAddNumberToObject(body, "mem_app_free", memCurrentlyFree());
                JAddNumberToObject(body, "mem_app_count", memAllocatedObjects());
                JAddNumberToObject(body, "mem_app_failures", memAllocationFailures());
                JAddItemToObject(rsp, "body", body);
                break;
            }

            // If body is specified, this request is from the test fixture to set
            // the test certificate in flash, else it's a request to display it.
            J *body = JGetObjectItem(req, "body");
            if (body != NULL) {
                JDetachItemViaPointer(req, body);
                err = postSelfTest(JGetBool(req, "verify"), body);
                if (err) {
                    JDelete(body);
                    break;
                }
                if (configp.tcert != NULL) {
                    const char *existingAppKey = JGetString(configp.tcert, tcFieldAppKey);
                    if (existingAppKey[0] != '\0') {
                        JDeleteItemFromObject(body, tcFieldAppKey);
                        JAddStringToObject(body, tcFieldAppKey, existingAppKey);
                    }
                    JDelete(configp.tcert);
                }
                configp.tcert = body;
                configpUpdate();
            }
            if (configp.tcert != NULL) {
                JAddItemToObject(rsp, "body", JDuplicate(configp.tcert, true));
            }
            break;
        }

        if (strEQL(reqtype, ReqCardRestart)) {
            reqDeferredWork = rdtRestart;
            break;
        }

        if (strEQL(reqtype, ReqCardBootloader)) {
            reqDeferredWork = rdtBootloader;
            break;
        }

        if (strEQL(reqtype, ReqCardRestore)) {
            if (!JGetBool(req, "delete")) {
                err = errF("cannot retain settings on a factory reset (must use \"delete\":true for full reset)");
                break;
            }
            reqDeferredWork = rdtRestore;
            break;
        }

        if (strEQL(reqtype, ReqCardTemp)) {
            MX_ADC_Init();
            double t = MX_ADC_GetTemperatureLevelAveraged();
            MX_ADC_DeInit();
            JAddNumberToObject(rsp, "value", t);
            break;
        }

        if (strEQL(reqtype, ReqCardVoltage)) {
            MX_ADC_Init();
            uint16_t batteryLevelmV = (uint16_t) MX_ADC_GetBatteryLevel();
            MX_ADC_DeInit();
            float vf = ((float) batteryLevelmV) / 1000;
            JAddNumberToObject(rsp, "value", vf);
            if (osUsbDetected()) {
                JAddBoolToObject(rsp, "usb", true);
            }
            break;
        }

        if (strEQL(reqtype, ReqFileDelete)) {
            J *files = JGetObjectItem(req, "files");
            J *item;
            JArrayForEach(item, files) {
                char *notefileID = (char *) JGetStringValue(item);
                if (!notefileIDIsReserved(notefileID)) {
                    pageDelete(PAGE_TYPE_NAMED, notefileID);
                }
            }
            break;
        }

        if (strEQL(reqtype, ReqCardLocation)) {
            JAddNumberToObject(rsp, "lat", config.lat);
            JAddNumberToObject(rsp, "lon", config.lon);
            JAddIntToObject(rsp, "time", config.ltime);
            break;
        }

        if (strEQL(reqtype, ReqCardLocationMode)) {
            bool updateConfig = false;
            if (JGetBool(req, "delete")) {
                config.lat = 0;
                config.lon = 0;
                config.ltime = 0;
                updateConfig = true;
            }
            char *mode = JGetString(req, "mode");
            if (mode[0] != '\0') {
                if (!streql(mode, "fixed")) {
                    err = errF("only 'fixed' location mode is supported");
                    break;
                }
                config.lat = JGetNumber(req, "lat");
                config.lon = JGetNumber(req, "lon");
                config.ltime = JGetNumber(req, "time");
                if (config.ltime == 0) {
                    config.ltime = timeSecs();
                }
                updateConfig = true;
            }
            JAddStringToObject(rsp, "mode", "fixed");
            if (updateConfig) {
                configUpdate();
            }
            break;
        }

        if (strEQL(reqtype, ReqFileChanges) || strEQL(reqtype, ReqFileGet) || strEQL(reqtype, ReqFileChangesPending)) {
            if (JGetString(req, "tracker")[0] != '\0') {
                err = errF("trackers not supported");
                break;
            }
            J *info, *filesArray = JGetObjectItem(req, "files");
            if (filesArray == NULL) {
                info = configTemplateGetNotefiles(false);
            } else {
                info = JCreateObject();
                J *item;
                JArrayForEach(item, filesArray) {
                    JAddItemToObject(info, (char *) JGetStringValue(item), JCreateObject());
                }
            }
            J *item;
            uint32_t grandTotal = 0;
            JObjectForEach(item, info) {
                const char *notefileID = JGetObjectItemName(item);
                J *nf = pageReadJSON(PAGE_TYPE_NAMED, (char *)notefileID);
                if (nf != NULL) {
                    J *note;
                    uint32_t total = 0;
                    JObjectForEach(note, nf) {
                        total++;
                    }
                    JAddIntToObject(item, "total", total);
                    grandTotal += total;
                } else {
                    JAddIntToObject(item, "total", 0);
                }
                if (strEQL(reqtype, ReqFileChangesPending)) {
                    JAddIntToObject(item, "changes", 0);
                }
            }
            uint32_t queueNotes;
            uint32_t pendingOutbound = xqpEnum(false, info, &queueNotes);
            if (pendingOutbound != 0) {
                grandTotal += queueNotes;
                if (strEQL(reqtype, ReqFileChangesPending)) {
                    JAddBoolToObject(rsp, "pending", true);
                    JAddIntToObject(rsp, "changes", pendingOutbound);
                }
            }
            if (JGetObjectItems(info) == 0) {
                JDelete(info);
            } else {
                JAddItemToObject(rsp, "info", info);
            }
            JAddIntToObject(rsp, "total", grandTotal);
            break;
        }

        if (strEQL(reqtype, ReqFileStats)) {
            char *filterNotefileID = JGetString(req, "file");
            J *info = configTemplateGetNotefiles(false);
            J *item;
            uint32_t grandTotal = 0;
            JObjectForEach(item, info) {
                const char *notefileID = JGetObjectItemName(item);
                if (filterNotefileID[0] == '\0' || streql(notefileID, filterNotefileID)) {
                    J *nf = pageReadJSON(PAGE_TYPE_NAMED, (char *)notefileID);
                    J *note;
                    JObjectForEach(note, nf) {
                        grandTotal++;
                    }
                }
            }
            JDelete(info);
            J *filter = NULL;
            if (filterNotefileID[0] != '\0') {
                filter = JCreateObject();
                J *to = JCreateObject();
                JAddItemToObject(filter, filterNotefileID, to);
            }
            uint32_t queueNotes;
            uint32_t pendingOutbound = xqpEnum(false, filter, &queueNotes);
            if (filter != NULL) {
                JDelete(filter);
            }
            grandTotal += queueNotes;
            JAddIntToObject(rsp, "changes", pendingOutbound);
            JAddIntToObject(rsp, "total", grandTotal);
            if (pendingOutbound) {
                JAddBoolToObject(rsp, "sync", true);
            }
            break;
        }

        if (strEQL(reqtype, ReqHubSync)) {
            char *mode = JGetString(req, "mode");
            if (strEQL(mode, "inbound")) {
                syncInbound(true);
            } else {
                syncNow();
            }
            break;
        }

        if (strEQL(reqtype, ReqHubSyncStatus)) {
            uint32_t nowMs = timerMs();
            uint32_t now = timeSecs();
            uint32_t completedSecs = syncCompletedMs == 0 ? 0 : (now + (nowMs - syncCompletedMs));
            uint32_t beganSecs = syncBeganMs == 0 ? 0 : (now + (nowMs - syncBeganMs));
            uint32_t requestedSecs = syncRequestedMs == 0 ? 0 : (now + (nowMs - syncRequestedMs));

            // Add stats that are independent of sync status
            if (completedSecs != 0) {
                JAddNumberToObject(rsp, "time", (uint32_t) completedSecs);
            }
            uint32_t pendingOutbound = xqpEnum(false, NULL, NULL);
            if (!timeIsValid() || pendingOutbound > 0) {
                JAddBoolToObject(rsp, "sync", true);
            }
            if (completedSecs != 0) {
                if (now > completedSecs) {
                    JAddNumberToObject(rsp, "completed", (uint32_t) (now - completedSecs));
                }
                JAddNumberToObject(rsp, "time", (uint32_t) completedSecs);
            }

            // Get status
            bool syncInProgress = false;
            buf[0] = '\0';
            if (beganSecs == 0) {
                // Has never synced
                strLcpy(buf, "waiting to begin first sync " ERR_IDLE);
            } else if (completedSecs == 0 || beganSecs > completedSecs) {
                // Sync in-progress
                strLcpy(buf, "sync in progress " STATUS_SYNC_IN_PROGRESS);
                syncInProgress = true;
            } else if (syncError != NULL) {
                // Sync completed but error
                JAddBoolToObject(rsp, "alert", true);
                strLcpy(buf, syncError);
                strLcat(buf, " " STATUS_SYNC_ERROR);
            } else {
                // Sync completed with no error
                strLcpy(buf, "sync completed " STATUS_SYNC_COMPLETED);
            }

            // Add requested but only if sync is not in progress
            if (requestedSecs != 0 && !syncInProgress) {
                if (now > requestedSecs) {
                    JAddNumberToObject(rsp, "requested", (uint32_t) (now - requestedSecs));
                }
            }

            // Add network failure to status if in penalty
            uint32_t penaltySecs = ledNetPenaltyPending();
            if (penaltySecs > 0) {
                JAddIntToObject(rsp, "seconds", penaltySecs);
                strLcat(buf, " " ERR_EXTENDED_NETWORK_FAILURE);
            }

            // Done
            JAddStringToObject(rsp, "status", buf);
            break;

        }

        if (strEQL(reqtype, ReqHubSet)) {
            bool updateConfig = false;
            bool updateConfigp = false;

            int inboundMins = JGetInt(req, "inbound");
            if (inboundMins != 0) {
                if (inboundMins < 0) {
                    inboundMins = 0;
                }
                if (config.inboundMins != inboundMins) {
                    config.inboundMins = inboundMins;
                    updateConfig = true;
                }
            }

            int outboundMins = JGetInt(req, "outbound");
            if (outboundMins != 0) {
                if (outboundMins < 0) {
                    outboundMins = 0;
                }
                if (config.outboundMins != outboundMins) {
                    config.outboundMins = outboundMins;
                    updateConfig = true;
                }
            }

            const char *productUID = JGetString(req, "product");
            if (productUID[0] != '\0') {
                // on loranote, for space savings the service provides the prefix
                if (memeql(productUID, "product:", 8)) {
                    productUID += 8;
                }
                if (!streql(config.productUID, productUID)) {
                    if (isResetKey(productUID)) {
                        productUID = "";
                    }
                    strLcpy(config.productUID, productUID);
                    updateConfig = true;
                    configInitialUplinksNeeded();
                }
            }

            const char *deviceSN = JGetString(req, "sn");
            if (deviceSN[0] != '\0') {
                if (!streql(config.deviceSN, deviceSN)) {
                    if (isResetKey(deviceSN)) {
                        productUID = "";
                    }
                    strLcpy(config.deviceSN, deviceSN);
                    updateConfig = true;
                    configInitialUplinksNeeded();
                }
            }

            const char *opMode = JGetString(req, "mode");
            if (opMode[0] != '\0') {
                if (isResetKey(opMode)) {
                    opMode = "";
                }
                uint32_t newMode;
                if (strEQL(opMode, "") || strEQL(opMode, "periodic")) {
                    newMode = OPMODE_PERIODIC;
                } else if (strEQL(opMode, "off")) {
                    newMode = OPMODE_OFF;
                } else if (strEQL(opMode, "continuous")) {
                    newMode = OPMODE_CONTINUOUS;
                } else if (strEQL(opMode, "minimum")) {
                    newMode = OPMODE_MINIMUM;
                } else {
                    err = errF("unrecognized mode");
                    break;
                }
                if (config.opMode != newMode) {
                    if (config.opMode == OPMODE_OFF) {
                        configInitialUplinksNeeded();
                    }
                    config.opMode = newMode;
                    updateConfig = true;
                }
            }

            J *details = JGetObjectItem(req, "details");
            if (details != NULL) {
                char *devEui = JGetString(details, "deveui");
                if (isResetKey(devEui)) {
                    devEui = "";
                }
                if (!streql(configp.devEui, devEui)) {
                    strLcpy(configp.devEui, devEui);
                    updateConfigp = true;
                }
                char *appEui = JGetString(details, "appeui");
                if (isResetKey(appEui)) {
                    appEui = "";
                }
                if (!streql(configp.appEui, appEui)) {
                    strLcpy(configp.appEui, appEui);
                    updateConfigp = true;
                }
                char *appKey = JGetString(details, "appkey");
                if (isResetKey(appKey)) {
                    appKey = "";
                }
                if (!streql(configp.appKey, appKey)) {
                    strLcpy(configp.appKey, appKey);
                    updateConfigp = true;
                }
            }

            if (updateConfig) {
                configUpdate();
            }
            if (updateConfigp) {
                configpUpdate();
            }

            // Make sure sync wakes up to perform uplinks
            taskGive(TASKID_SYNC);
            break;
        }

        if (strEQL(reqtype, ReqCardIO)) {
            bool updateConfigp = false;
            int i2cAddress = JGetInt(req, "i2c");
            if (i2cAddress != 0) {
                if (i2cAddress < 0 || i2cAddress >= 127) {
                    configp.i2cAddress = 0;
                } else {
                    configp.i2cAddress = i2cAddress;
                }
                updateConfigp = true;
            }
            JAddIntToObject(rsp, "i2c", configp.i2cAddress);
            if (updateConfigp) {
                configpUpdate();
            }
            break;
        }

        if (strEQL(reqtype, ReqCardTime)) {
            JAddIntToObject(rsp, "time", timeSecs());
            break;
        }

        if (strEQL(reqtype, ReqCardStatus)) {
            JAddIntToObject(rsp, "time", timeSecsBoot());
            if (osUsbDetected()) {
                JAddBoolToObject(rsp, "usb", true);
            }
            // Undocumented option to show time since last restart based on a timer, not RTC
            if (JGetBool(req, "verify")) {
                JAddIntToObject(rsp, "seconds", (int) (timerMsSinceBoot() / ms1Sec));
            }
            break;
        }

        if (strEQL(reqtype, ReqCardAux)) {
            bool updateConfig = false;
            char *mode = JGetString(req, "mode");
            if (strEQL(mode, "off") || isResetKey(mode)) {
                if (config.neomon) {
                    config.neomon = false;
                    updateConfig = true;
                }
                if (config.gpio) {
                    config.gpio = false;
                    updateConfig = true;
                    gstateRefresh();
                }
            }
            if (strEQL(mode, "neo-monitor")) {
                if (config.gpio) {
                    config.gpio = false;
                    updateConfig = true;
                    gstateRefresh();
                }
                int sensitivity = JGetInt(req, "sensitivity");
                if (config.neobrt != sensitivity) {
                    config.neobrt = sensitivity;
                    neoSetBrightness(config.neobrt);
                    updateConfig = true;
                }
                if (!config.neomon) {
                    config.neomon = true;
                    neoSetBrightness(config.neobrt);
                    updateConfig = true;
                }
            }
            if (strEQL(mode, "gpio")) {
                if (config.neomon) {
                    config.neomon = false;
                    updateConfig = true;
                }
                bool stateChange = false;
                J *item, *usageArray = JGetObjectItem(req, "usage");
                buf[0] = '\0';
                JArrayForEach(item, usageArray) {
                    if (buf[0] != '\0') {
                        strLcat(buf, ",");
                    }
                    strLcat(buf, (char *) JGetStringValue(item));
                }
                if (!streql(config.gpioUsage, buf)) {
                    strLcpy(config.gpioUsage, buf);
                    updateConfig = true;
                    stateChange = true;
                }
                if (!config.gpio) {
                    config.gpio = true;
                    updateConfig = true;
                    stateChange = true;
                }
                if (stateChange) {
                    // Any invalid state will be returned as an error parsing
                    // config.gpioUsage when we are refreshing the state.
                    err = gstateRefresh();
                }
            }
            if (updateConfig) {
                configUpdate();
            }
            if (config.neomon) {
                JAddStringToObject(rsp, "mode", "neo-monitor");
            }
            if (config.gpio) {
                JAddStringToObject(rsp, "mode", "gpio");
                JAddStringToObject(rsp, "usage", config.gpioUsage);
                J *pinState = gstateGet();
                if (pinState != NULL) {
                    JAddItemToObject(rsp, "state", pinState);
                }
            }
            break;
        }

        if (strEQL(reqtype, ReqCardTrace)) {
            char *mode = JGetString(req, "mode");
            if (strEQL(mode, "sync-trace-on") || strEQL(mode, "synctrace") || strEQL(mode, "+synctrace")) {
                synclogTrace(true, JGetInt(req, "max"));
            }
            if (strEQL(mode, "sync-trace-off") || strEQL(mode, "-synctrace")) {
                synclogTrace(false, 0);
            }
            break;
        }

        if (strEQL(reqtype, ReqCardVersion)) {
            configGetDeviceUID(buf, sizeof(buf));
            JAddStringToObject(rsp, "device", buf);
            JAddStringToObject(rsp, "name", PRODUCT_MANUFACTURER " " PRODUCT_DISPLAY_NAME);
            snprintf(buf, sizeof(buf), "%d", CURRENT_BOARD);
            JAddStringToObject(rsp, "board", buf);
            snprintf(buf, sizeof(buf), "%s-%d.%d.%d.%d", PRODUCT_PROGRAMMATIC_NAME, PRODUCT_MAJOR_VERSION, PRODUCT_MINOR_VERSION, PRODUCT_PATCH_VERSION, osBuildNum());
            JAddStringToObject(rsp, "version", buf);
            if (configp.tcert != NULL) {
                JAddStringToObject(rsp, "sku", JGetString(configp.tcert, tcFieldSKU));
            }
            J *info = JParse(osBuildConfig());
            if (info != NULL) {
                JAddItemToObject(rsp, "body", info);
            }
            break;
        }

        if (strEQL(reqtype, ReqHubGet)) {
            configGetDeviceUID(buf, sizeof(buf));
            JAddStringToObject(rsp, "device", buf);
            JAddStringToObject(rsp, "product", config.productUID);
            JAddStringToObject(rsp, "sn", config.deviceSN);
            JAddIntToObject(rsp, "outbound", config.outboundMins);
            JAddIntToObject(rsp, "inbound", config.inboundMins);
            const char *mode;
            switch (config.opMode) {
            case OPMODE_OFF:
                mode = "off";
                break;
            case OPMODE_PERIODIC:
                mode = "periodic";
                break;
            case OPMODE_CONTINUOUS:
                mode = "continuous";
                break;
            case OPMODE_MINIMUM:
                mode = "minimum";
                break;
            default:
                mode = "?";
                break;
            }
            JAddStringToObject(rsp, "mode", mode);
            JAddIntToObject(rsp, "inbound", config.inboundMins);
            JAddIntToObject(rsp, "outbound", config.inboundMins);
            if (configp.devEui[0] != '\0' || configp.appEui[0] != '\0' || configp.appKey[0] != '\0') {
                J *details = JCreateObject();
                JAddStringToObject(details, "deveui", configp.devEui);
                JAddStringToObject(details, "appeui", configp.appEui);
                JAddStringToObject(details, "appkey", configp.appKey);
                JAddItemToObject(rsp, "details", details);
            }
            break;
        }

        if (strEQL(reqtype, ReqHubStatus)) {
            buf[0] = '\0';
            if (config.opMode == OPMODE_OFF) {
                snprintf(buf, sizeof(buf), "network unavailable because mode is 'off' " ERR_TRANSPORT_UNAVAILABLE " " ERR_IDLE);
            } else if (loraAppJoined()) {
                if (config.opMode == OPMODE_PERIODIC || !loraAppClearToSend()) {
                    JAddBoolToObject(rsp, "connected", true);
                    snprintf(buf, sizeof(buf), "connected to LoRaWAN network " ERR_TRANSPORT_CONNECTED);
                } else {
                    snprintf(buf, sizeof(buf), "idle " ERR_IDLE " " ERR_TRANSPORT_DISCONNECTED);
                }
            } else if (!loraAppClearToSend()) {
                snprintf(buf, sizeof(buf), "attempting to join LoRaWAN network " ERR_TRANSPORT_CONNECTING " " ERR_TRANSPORT_WAIT_SERVICE);
            } else if (ledNetErrorPending()) {
                snprintf(buf, sizeof(buf), "LoRaWAN network connect failure " ERR_NETWORK " " ERR_TRANSPORT_CONNECT_FAILURE " " ERR_IDLE);
            }
            if (ledNetPenaltyPending()) {
                strLcat(buf, " " ERR_EXTENDED_NETWORK_FAILURE);
            }
            JAddStringToObject(rsp, "status", buf);
            break;
        }

        if (strEQL(reqtype, ReqNoteTemplate) || strEQL(reqtype, ReqEnvTemplate)) {
            J *body = JGetObjectItem(req, "body");
            char *notefileID;
            int port;
            bool confirmed;
            bool isEnvTemplate = false;
            if (strEQL(reqtype, ReqEnvTemplate)) {
                isEnvTemplate = true;
                notefileID = sysNotefileEnv;
                port = LPORT_SYS_NOTEFILE_ENV;
                confirmed = true;
                if (body == NULL) {
                    body = JCreateObject();
                    JAddItemToObject(req, "body", body);
                }
                configAddSysEnvToTemplateBody(body);
            } else {
                notefileID = JGetString(req, "file");
                if (notefileID[0] == '\0') {
                    err = errF("notefile ID is required");
                    break;
                }
                if (notefileIDIsReserved(notefileID)) {
                    err = errF("notefile ID is reserved");
                    break;
                }
                confirmed = JGetBool(req, "confirm");
                port = (int) JGetInt(req, "port");
                if (port == 0 && body != NULL) {
                    err = errF("please specify a unique 'port' number to be used by this notefile (from %d to %d)", LPORT_USER_NOTEFILE_FIRST, LPORT_USER_NOTEFILE_LAST);
                    break;
                }
            }
            bool isQueue, outbound, inbound;
            err = notefileAttributesFromID(notefileID, &isQueue, &outbound, &inbound, NULL);
            if (err) {
                break;
            }
            char usedBy[maxNotefileID];
            err = configTemplateIsPortUsed(notefileID, (uint8_t) port, inbound, outbound, usedBy);
            if (err) {
                configTemplateDelete(usedBy);
                snprintf(buf, sizeof(buf), "%s: overridden", errString(err));
                JAddStringToObject(rsp, "status", buf);
                break;
            }
            err = binaryValidateTemplate(body);
            if (err) {
                break;
            }
            if (body == NULL) {
                err = configTemplateDelete(notefileID);
                if (err) {
                    break;
                }
                configUpdate();
            } else {
                if (!isQueue && !isEnvTemplate) {
                    // Add this so that noteID is included as a part of .db bodies
                    JAddStringToObject(body, "_note", "0");
                }
                char *template = JPrintUnformattedOmitEmpty(body);
                err = configTemplateSet(notefileID, (uint8_t) port, confirmed, inbound, outbound, template, true);
                memFree(template);
                if (err) {
                    break;
                }
                configUpdate();
                configInitialUplinksNeeded();
            }
            break;

        }

        if (strEQL(reqtype, ReqNoteGet) || strEQL(reqtype, ReqNoteDelete)) {
            bool delete = strEQL(reqtype, ReqNoteDelete);
            if (JGetBool(req, "delete")) {
                delete = true;
            }

            char *notefileID = JGetString(req, "file");
            if (notefileID[0] == '\0') {
                err = errF("no notefile specified");
                break;
            }

            // Handle synclog pseudo-file
            if (strEQL(notefileID, hubSynclogNotefile)) {
                if (config.opMode != OPMODE_OFF && loraAppJoined()) {
                    JAddBoolToObject(rsp, "connected", true);
                }
                if (JGetBool(req, "start")) {
                    JAddStringToObject(rsp, "status", SSENUM);
                } else {
                    J *body = synclogDequeue(JGetBool(req, "delete"));
                    if (body == NULL) {
                        err = errF("sync log is empty " ERR_NOTE_NOEXIST);
                        break;
                    } else {
                        JAddItemToObject(rsp, "body", body);
                    }
                }
                break;
            }

            // Disallow other reserved files
            if (notefileIDIsReserved(notefileID)) {
                err = errF("reserved notefile name");
                break;
            }

            // Do checks based on type
            char noteID[maxNoteID];
            bool isQueue, outbound, inbound;
            err = notefileAttributesFromID(notefileID, &isQueue, &outbound, &inbound, NULL);
            if (isQueue) {
                noteID[0] = '\0';
                if (outbound) {
                    err = errF("can't %s notes from an outbound queue", reqtype);
                    break;
                }
            } else {
                strLcpy(noteID, JGetString(req, "note"));
                if (noteID[0] == '\0') {
                    err = errF("note ID must be specified");
                    break;
                }
            }

            // Get the note
            J *body, *payload;
            err = notefileGetNote(notefileID, noteID, delete, &body, &payload);
            if (err) {
                break;
            }

            // Done
            if (strEQL(reqtype, ReqNoteDelete)) {
                JDelete(body);
                JDelete(payload);
            } else {
                if (!isQueue) {
                    JAddStringToObject(rsp, "note", noteID);
                }
                if (body != NULL) {
                    JAddItemToObject(rsp, "body", body);
                }
                if (payload != NULL) {
                    JAddItemToObject(rsp, "payload", payload);
                }
            }
            break;

        }

        if (strEQL(reqtype, ReqEnvModified)) {
            JAddIntToObject(rsp, "time", envModifiedTime());
            break;
        }

        if (strEQL(reqtype, ReqEnvGet)) {
            int conditionalGetTime = JGetInt(req, "time");
            if (conditionalGetTime != 0) {
                if (conditionalGetTime == envModifiedTime()) {
                    err = errF("environment hasn't been modified " ERR_ENV_NOT_MODIFIED);
                    break;
                }
            }
            char *key = JGetString(req, "name");
            if (key[0] != '\0') {
                J *new = envGetString(key);
                if (new == NULL) {
                    err = errF("env var cannot be found in " ReqEnvTemplate);
                    break;
                }
                JAddItemToObject(rsp, "text", new);
            } else {
                envGetAll(rsp, JGetObjectItem(req, "names"));
            }
            break;
        }

        if (strEQL(reqtype, ReqEnvDefault)) {
            bool updateEnvironment = false;
            char *notefileID = sysNotefileUserEnvDefault;
            char *noteID = JGetString(req, "name");
            if (noteID[0] == '\0') {
                err = errF("no environment variable name specified");
                break;
            }
            char *value = JGetString(req, "text");
            if (value[0] == '\0') {
                J *body, *payload;
                if (notefileGetNote(notefileID, noteID, true, &body, &payload) == errNone) {
                    JDelete(body);
                    JDelete(payload);
                    updateEnvironment = true;
                }
            } else {
                J *body = JCreateObject();
                JAddStringToObject(body, "text", value);
                err = notefileAddNote(notefileID, false, noteID, true, NULL, body, NULL, 0, 0, false, false, false, NULL, NULL);
                if (err) {
                    break;
                }
                updateEnvironment = true;
            }
            if (updateEnvironment) {
                envUpdate();
            }
            break;
        }

        if (strEQL(reqtype, ReqNoteAdd) || strEQL(reqtype, ReqNoteUpdate)) {
            bool updateEnvironment = false;
            bool update = strEQL(reqtype, ReqNoteUpdate);
            char *notefileID, *noteID;
            if (!JIsPresent(req, "body") && !JIsPresent(req, "payload")) {
                err = errF("please supply a body, a payload, or both " ERR_SYNTAX);
                break;
            }
            J *body;
            noteID = JGetString(req, "note");
            notefileID = JGetString(req, "file");
            if (notefileID[0] == '\0') {
                notefileID = "data.qo";
            }
            if (notefileIDIsReserved(notefileID)) {
                err = errF("reserved notefile name");
                break;
            }
            body = JGetObjectItem(req, "body");
            if (body == NULL) {
                body = JCreateObject();
            } else {
                JDetachItemViaPointer(req, body);
            }
            uint8_t port;
            bool confirmed, inbound, outbound, isQueue;
            err = notefileAttributesFromID(notefileID, &isQueue, &outbound, &inbound, NULL);
            if (err) {
                JDelete(body);
                break;
            }
            if (noteID[0] == '\0' && !update && !isQueue) {
                err = errF("no note ID specified");
                JDelete(body);
                break;
            }
            if (isQueue && inbound) {
                err = errF("can't add note to an inbound queue " ERR_INCOMPATIBLE);
                JDelete(body);
                break;
            }
            J *template = NULL;
            if (inbound || outbound) {
                err = configTemplateGetObject(notefileID, &template, &port, &confirmed, &inbound, &outbound);
                if (err) {
                    JDelete(body);
                    break;
                }
            }
            uint8_t *payload;
            uint32_t payloadLen;
            if (!JGetBinaryFromObject(req, "payload", &payload, &payloadLen)) {
                payload = NULL;
                payloadLen = 0;
            }
            bool addNoteIDToResponse = false;
            char randomNoteID[maxNoteID];
            if (strEQL(noteID, "?")) {
                snprintf(randomNoteID, sizeof(randomNoteID), "%lX", prandNumber());
                noteID = randomNoteID;
                addNoteIDToResponse = true;
            }
            // Note that both "body", and "payload" are deleted by notefileAddNote even on error paths
            uint32_t total, bytes;
            err = notefileAddNote(notefileID, isQueue, noteID, update, template, body, payload, payloadLen, port, confirmed, inbound, outbound, &total, &bytes);
            JDelete(template);
            if (err) {
                break;
            }
            if (JGetBool(req, "sync")) {
                syncNow();
            }
            if (addNoteIDToResponse) {
                JAddStringToObject(rsp, "note", noteID);
            }
            if (!update) {
                JAddIntToObject(rsp, "bytes", bytes);
                if (total != 0) {
                    JAddIntToObject(rsp, "total", total);
                }
            }
            if (updateEnvironment) {
                envUpdate();
            }
            break;
        }

        if (strEQL(reqtype, ReqCardAttn)) {

            // If there's a payload that the external MCU wishes for us to hold onto
            // while they shut down, for later retrieval on restart, process it.
            if (JGetString(req, "payload")[0] != '\0' || JGetBool(req, "start")) {
                err = errF(ReqCardAttn " payloads not supported");
                break;
            }

            // Perform different actions based on mode
            bool performSleep = false;
            bool performArm = false;
            bool performRearm = false;
            bool performDisarm = false;
            bool disableRequestPins = false;

            // First, see if arm or disarm are in the list
            char *mode = JGetString(req, "mode");
            strlcpy(buf, mode, sizeof(buf));
            char *p = buf;
            while (!err && p != NULL && *p != '\0') {
                char *next = strchr(p, ',');
                if (next != NULL) {
                    *next++ = '\0';
                }
                if (*p == '+') {
                    p++;
                }
                if (strEQL(p, "reset") || strEQL(p, "arm") || strEQL(p, "rearm") || strEQL(p, "-disarm")
                        || strEQL(p, "sleep") || strEQL(p, "sleep-disable") ) {
                    if (strEQL(p, "sleep")) {
                        performSleep = true;
                    }
                    if (strEQL(p, "sleep-disable")) {
                        performSleep = true;
                        disableRequestPins = true;
                    }
                    if (performDisarm) {
                        err = errF("arm cannot be combined with disarm");
                    } else {
                        performArm = true;
                        performRearm = strEQL(p, "rearm");
                    }
                } else if (strEQL(p, "set") || strEQL(p, "disarm") || strEQL(p, "-arm") || strEQL(p, "wake")) {
                    if (performArm) {
                        err = errF("disarm cannot be combined with arm");
                    } else {
                        performDisarm = true;
                    }
                }
                p = next;
            }

            // If doing arm, disarm before anything else
            if ((performArm && !performRearm) || performDisarm) {
                attnClearReset();
            }

            // Iterate over comma-separated keywords other than arm/disarm
            strlcpy(buf, mode, sizeof(buf));
            p = buf;
            while (!err && p != NULL && *p != '\0') {

                // Iterate over comma-separated keywords
                char *next = strchr(p, ',');
                if (next != NULL) {
                    *next++ = '\0';
                }

                // Skip if it is + because that's implicit
                if (*p == '+') {
                    p++;
                }

                // See if "files" item is present
                J *files = JGetObjectItem(req, "files");

                // Process this keyword
                if (strEQL(p, "reset") || strEQL(p, "arm") || strEQL(p, "rearm") || strEQL(p, "-disarm") || strEQL(p, "sleep") || strEQL(p, "sleep-disable") ) {
                    // handled earlier
                } else if (strEQL(p, "set") || strEQL(p, "disarm") || strEQL(p, "wake") || strEQL(p, "-arm")) {
                    // handled earlier
                } else if (strEQL(p, "-all")) {
                    err = attnDisableAll();
                } else if (strEQL(p, "-files")) {
                    err = attnDisableModified();
                } else if (strEQL(p, "files") || (files != NULL && JGetArraySize(files) > 0)) {
                    err = attnEnableModified(files);
                    if (!err) {
                        files = NULL;
                    }
                } else if (strEQL(p, "-usb")) {
                    err = attnDisableUSB();
                } else if (strEQL(p, "usb")) {
                    err = attnEnableUSB();
                } else if (strEQL(p, "-env")) {
                    err = attnDisableENV();
                } else if (strEQL(p, "env")) {
                    err = attnEnableENV();
                } else if (strEQL(p, "-auxgpio")) {
                    err = attnDisableAuxGPIO();
                } else if (strEQL(p, "auxgpio")) {
                    err = attnEnableAuxGPIO();
                } else if (p[0] != '\0') {
                    err = errF("attn mode not supported: %s " ERR_SYNTAX, p);
                }

                // Next keyword
                p = next;
            }
            if (!err && performArm) {
                int secs = JGetInt(req, "seconds");
                if (!debugPort) {
                    debugf("ARMED FOR %d SECONDS\n", secs);
                }
                err = attnReset(secs, performSleep, disableRequestPins);
            }
            if (err) {
                break;
            }

            // Return the state of the ATTN pin to the caller.  We read the
            // hardware pin rather than doing it in software so that our tests
            // can use this as a way of validating the state of the pin.
            if (attnIsArmed()) {
                JAddBoolToObject(rsp, "set", true);
            }

            // Return the events to the caller
            int32_t secs;
            J *attnEvents;
            err = attnGetEvents(&attnEvents, JGetBool(req, "verify"), &secs);
            if (err) {
                break;
            }
            JAddIntToObject(rsp, "seconds", secs);
            if (attnEvents != NULL) {
                if (JGetArraySize(attnEvents) == 0) {
                    JDelete(attnEvents);
                } else {
                    JAddItemToObject(rsp, "files", attnEvents);
                }
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

    // Return successful response
    if (rspJSON == NULL) {
        JDelete(rsp);
    } else {
        char *json = JPrintUnformattedOmitEmpty(rsp);
        JDelete(rsp);
        *rspJSON = (uint8_t *) json;
        *rspJSONLen = strlen(json);
        if (config.traceRequests || configp.traceRequests) {
            if (!debugPort) {
                debugMessage("<< ");
                debugMessage(json);
                debugMessage("\n");
            }
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
