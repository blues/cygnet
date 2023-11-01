
#include "app.h"

// Maximum command
#define maxCMD 256

// Commands
typedef enum {
    CMD_RESTART,
    CMD_POWER,
    CMD_MEM,
    CMD_M,
    CMD_TEST,
    CMD_BOOTLOADER_DIRECT,
    CMD_UNRECOGNIZED
} allCommands;

typedef struct {
    char *cmd;
    int ID;
} cmd_def;

STATIC const cmd_def cmdText[] = {
    {"restart", CMD_RESTART},
    {"mem", CMD_MEM},
    {"m", CMD_M},
    {"test", CMD_TEST},
    {"power", CMD_POWER},
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
    UNUSED_PARAMETER(argv);
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
        MX_JumpToBootloader();
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
        MX_ActivePeripherals(buf, sizeof(buf));
        debugf("POWER: %s\n", buf);
        break;
    }

    case CMD_M: {
        serialSendLineToModem(&cmdline[2]);
        break;
    }

    case CMD_TEST: {
        for (int i=0; i<2; i++) {
            serialSendLineToModem("ATE0");
            timerMsSleep(500);
        }
        serialSendLineToModem("AT+QSCLK=0");    // Turn off 10 second auto sleep timer
        timerMsSleep(500);
        serialSendLineToModem("AT+CIMI");       // Get IMSI (returns ERROR if SIM not plugged in)
        timerMsSleep(500);
        serialSendLineToModem("AT+QGMR");       // Get firmware version
        timerMsSleep(500);
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
