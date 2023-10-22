
#include "notelib.h"
#include "main.h"
#include "lora_conf.h"

// Maximum command
#define maxCMD 256

// Commands
typedef enum {
    CMD_FACTORY,
    CMD_PRODUCT,
    CMD_RESTART,
    CMD_T,
    CMD_MEM,
    CMD_NEOTEST,
    CMD_QO,
    CMD_PING,
    CMD_TIME,
    CMD_JOIN,
    CMD_DUMP,
    CMD_SYNC,
    CMD_ENV,
    CMD_TRACE,
    CMD_CONFIG,
    CMD_POWER,
    CMD_BOOTLOADER_DIRECT,
    CMD_UNRECOGNIZED
} allCommands;

typedef struct {
    char *cmd;
    int ID;
} cmd_def;

STATIC const cmd_def cmdText[] = {
    {"factory", CMD_FACTORY},
    {"restart", CMD_RESTART},
    {"t", CMD_T},
    {"mem", CMD_MEM},
    {"qo", CMD_QO},
    {"neotest", CMD_NEOTEST},
    {"dump", CMD_DUMP},
    {"product", CMD_PRODUCT},
    {"sync", CMD_SYNC},
    {"ping", CMD_PING},
    {"time", CMD_TIME},
    {"join", CMD_JOIN},
    {"env", CMD_ENV},
    {"config", CMD_CONFIG},
    {"power", CMD_POWER},
    {"trace", CMD_TRACE},
    {"bootloader", CMD_BOOTLOADER_DIRECT},
    {"zxc", CMD_BOOTLOADER_DIRECT},
    {NULL, 0},
};

// Forwards
int asciiEQLCI(char *x, char* y);
int asciiEQLCIDelim(char *x, char* y, int yLen);
int getCommand(char *cmd, int cmdLen);

// Process a diagnostic command
err_t diagProcess(char *diagCommand)
{
    err_t err = errNone;

    // Don't allow debug output that could interfere with JSON requests
    bool debugWasEnabled = MX_DBG_Enable(true);

    // Get the command, or continue with the mode
    uint32_t diagCommandLen = strlen(diagCommand);
    int cmd = getCommand(diagCommand, diagCommandLen);

    // Copy to a local buffer, cleaning and null-terminated for string processing
    char argbuf[maxCMD];
    int j = 0;
    for (int i=0; i<diagCommandLen; i++) {
        if (j >= sizeof(argbuf)-1) {
            break;
        }
        bool goodChar = false;
        if (isAsciiAlphaNumeric(diagCommand[i])) {
            goodChar = true;
        }
        if (ispunct(diagCommand[i]) || diagCommand[i] == ' ') {
            goodChar = true;
        }
        if (goodChar) {
            argbuf[j++] = diagCommand[i];
        }
    }
    argbuf[j] = '\0';

    // Retain the argbuf before we write nulls into it
    char cmdline[maxCMD];
    strLcpy(cmdline, argbuf);

    // Break into an argv list
    char *argv[6];
    int argvn[6];
    char *prev = argbuf;
    int argc = 0;
    for (int i=0; i<(sizeof(argv)/sizeof(argv[0])); i++) {
        argv[i] = "";
    }
    for (int i=0; i<(sizeof(argvn)/sizeof(argvn[0])); i++) {
        argvn[i] = 0;
    }
    while (true) {
        argv[argc] = prev;
        argvn[argc] = atoi(prev);
        argc++;
        if (argc >= (sizeof(argv) / sizeof(argv[0]))) {
            break;
        }
        char *next1 = strstr(prev, " ");
        char *next2 = strstr(prev, ",");
        if (next1 == NULL) {
            next1 = next2;
        }
        char *next = next1;
        if (next1 != NULL && next2 != NULL && next2 < next1) {
            next = next2;
        }
        if (next == NULL) {
            break;
        }
        *next++ = '\0';
        prev = next;
    }
    UNUSED_VARIABLE(argvn);

    // Process recognized commands
    switch (cmd) {

    case CMD_BOOTLOADER_DIRECT: {
        // On Mac, once this is done you can use stm32cubeprogrammer to
        // connect to the CP2102 in UART mode, at 921600 baud rate and
        // with EVEN parity.  (You cannot connect via "USB" as we do with
        // the normal Notecard because, unlike the stm32l4r5 or u5, we
        // are using a CP2102 to serve USB and thus the VID/PID of the
        // stm32's bootloader isn't directly exposed.  Thus UART not USB.
        MY_JumpToBootloaderDirect();
        debugPanic("boot");
        break;
    }

    case CMD_MEM: {
        debugf("RAM   physical: %lu\n", heapPhysical);
        debugf("RAM at startup: %lu\n", heapFreeAtStartup);
        debugf("RAM       free: %lu\n", xPortGetFreeHeapSize());
        taskStackStats();
        break;
    }

    case CMD_POWER: {
        char buf[100];
        MY_ActivePeripherals(buf, sizeof(buf));
        debugf("POWER: %s\n", buf);
        break;
    }

    case CMD_T: {
        argv[1] = "on";
    }
    __fallthrough;
    case CMD_TRACE:
        if (asciiEQLCI(argv[1], "off")) {
            if (debugWasEnabled) {
                debugWasEnabled = false;
                config.traceLevel = 0;
                configUpdate();
            }
        } else if (asciiEQLCI(argv[1], "on")) {
            if (!debugWasEnabled) {
                debugWasEnabled = true;
                config.traceLevel = 1;
                configUpdate();
            }
        } else if (asciiEQLCI(argv[1], "-req") && config.traceRequests != 0) {
            config.traceRequests = 0;
            configUpdate();
        } else if (asciiEQLCI(argv[1], "+req") && config.traceRequests == 0) {
            config.traceRequests = 1;
            configUpdate();
        } else if (asciiEQLCI(argv[1], "-reqp") && configp.traceRequests != 0) {
            configp.traceRequests = 0;
            configpUpdate();
        } else if (asciiEQLCI(argv[1], "+reqp") && configp.traceRequests == 0) {
            configp.traceRequests = 1;
            configpUpdate();
        }
        debugf("trace is now %s", config.traceLevel ? "on" : "off");
        if (config.traceRequests) {
            debugR(" +req");
        }
        if (configp.traceRequests) {
            debugR(" +reqp");
        }
        debugR("\n");
        break;

    case CMD_QO:
        xqpEnum(true, NULL, NULL);
        break;

    case CMD_SYNC:
        syncNow();
        break;

    case CMD_PING:
        syncPing();
        break;

    case CMD_NEOTEST:
        if (argvn[1] != 0) {
            neoSetBrightness(argvn[1]);
            break;
        }
        neoVLED(VLED_GRAY, false);
        neoVLED(VLED_MAGENTA, false);
        neoVLED(VLED_ORANGE, false);
        neoVLED(VLED_BLUE, false);
        neoVLED(VLED_YELLOW, false);
        neoVLED(VLED_RED, false);
        neoVLED(VLED_CYAN, false);
        neoVLED(VLED_GREEN, false);
        neoVLED(VLED_WHITE, false);
        if (argv[1][0] == 'v') {
            for (int i=0; i<VLED_TYPES; i++) {
                neoVLED(i, true);
                timerMsSleep(500);
                neoVLED(i, false);
                timerMsSleep(500);
            }
        } else if (argv[1][0] == '\0') {
            neoVLED(VLED_WHITE, true);
            timerMsSleep(2500);
            neoVLED(VLED_WHITE, false);
        } else {
            neoTest(argv[1], argvn[2]);
        }
        break;

    case CMD_TIME: {
        static uint32_t referenceTime, referenceTimeSetAt = 0;
        int64_t ms = timerMs();
        uint32_t s = timeSecs();
        char timestr[48];
        timeDateStr(s, timestr, sizeof(timestr));
        if (argvn[1] != 0) {
            referenceTimeSetAt = timeSecs();
            referenceTime = argvn[1];
        }
        debugf("%s\n%d\n%lld\n", timestr, s, ms);
        if (referenceTimeSetAt != 0) {
            debugf("drift test: %d\n", referenceTime + (s - referenceTimeSetAt));
        }
        break;
    }

    case CMD_JOIN:
        loraAppJoin();
        break;

    case CMD_ENV: {
        J *rsp = JCreateObject();
        envGetAll(rsp, NULL);
        char *json = JPrint(rsp);
        debugMessage(json);
        debugMessage("\n");
        memFree(json);
        JDelete(rsp);
        break;
    }

    case CMD_FACTORY:
        pageFactoryReset();
        debugPanic("factory reset");
        break;

    case CMD_RESTART:
        debugPanic("intentional restart");
        break;

    case CMD_PRODUCT:
        if (argv[1][0] != '\0') {
            char buf[128];
            snprintf(buf, sizeof(buf), "{\"req\":\"hub.set\",\"product\":\"%s\"}", argv[1]);
            err = reqProcess(true, (uint8_t *)buf, strlen(buf), false, NULL, NULL);
            if (err) {
                break;
            }
        }
        debugf("productUID: '%s'\n", config.productUID);
        break;

    case CMD_DUMP:
        if (argv[1][0] == '\0') {
            debugf("notefileID?\n");
        } else {
            J *j = pageReadJSON(PAGE_TYPE_NAMED, argv[1]);
            if (j == NULL) {
                debugf("page read failed\n");
            } else {
                char *json = JPrint(j);
                debugf("/// %s\n", argv[1]);
                debugMessage(json);
                debugMessage("\n///\n");
                memFree(json);
                JDelete(j);
            }
        }
        break;

    case CMD_CONFIG: {
        J *j = pageReadJSON(PAGE_TYPE_CONFIG, NULL);
        if (j != NULL) {
            char *json = JPrint(j);
            debugf("/// %d bytes\n", strlen(json)+1);
            debugMessage(json);
            debugMessage("\n///\n");
            memFree(json);
            JDelete(j);
        }
        break;
    }

    case CMD_UNRECOGNIZED:
        debugf("unrecognized command\n");
        break;

    }

    if (err) {
        debugf("%s\n", errString(err));
    }

    // Restore debug output
    MX_DBG_Enable(debugWasEnabled);

    // Done
    return errNone;

}

// Returns 1 if x == y (case insensitive)
int asciiEQLCI(char *x, char* y)
{
    while (*x && *y) {
        if ((*x++ & 0xdf) != (*y++ & 0xdf)) {
            return 0;
        }
    }
    return (*x == 0) && (*y == 0);
}

// Returns 1 if x == y (case insensitive) with a non-string second arg
int asciiEQLCIDelim(char *x, char* y, int yLen)
{
    int i;
    for (i=0; *x && i<yLen; i++) {
        if ((*x++ & 0xdf) != (*y++ & 0xdf)) {
            return 0;
        }
    }
    return (*x == 0) && (i == yLen || *y == ' ');
}

// See if a command is recognized
int getCommand(char *cmd, int cmdLen)
{
    for (int i=0;; i++) {
        if (cmdText[i].cmd == NULL) {
            break;
        }
        if (asciiEQLCIDelim(cmdText[i].cmd, cmd, cmdLen)) {
            return cmdText[i].ID;
        }
    }
    return CMD_UNRECOGNIZED;
}
