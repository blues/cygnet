
#include "main.h"
#include "mutex.h"
#include "json.h"
#include "product.h"
#include "request.h"

#pragma once

// Task parameters
#define TASKID_MAIN                 0           // Serial uart poller
#define TASKNAME_MAIN               "uart"
#define TASKLETTER_MAIN             'U'
#define TASKSTACK_MAIN              2000
#define TASKPRI_MAIN                ( configMAX_PRIORITIES - 1 )        // highest

#define TASKID_REQ                  1           // Request handler
#define TASKNAME_REQ                "request"
#define TASKLETTER_REQ              'R'
#define TASKSTACK_REQ               2500
#define TASKPRI_REQ                 ( configMAX_PRIORITIES - 2 )        // Normal

#define TASKID_MON                  2           // Monitor/poller
#define TASKNAME_MON                "monitor"
#define TASKLETTER_MON              'M'
#define TASKSTACK_MON               2500
#define TASKPRI_MON                 ( configMAX_PRIORITIES - 2 )        // Normal

#define TASKID_MODEM                3           // Modem
#define TASKNAME_MODEM              "modem"
#define TASKLETTER_MODEM            'Q'
#define TASKSTACK_MODEM             2500
#define TASKPRI_MODEM               ( configMAX_PRIORITIES - 2 )        // Normal

#define TASKID_NUM_TASKS            4           // Total
#define TASKID_UNKNOWN              0xFFFF
#define STACKWORDS(x)               ((x) / sizeof(StackType_t))

// Test certificate
#define tcFieldDeviceUID                "device"
#define tcFieldError                    "err"
#define tcFieldStatus                   "status"
#define tcFieldTests                    "tests"
#define tcFieldFailTest                 "fail_test"
#define tcFieldFailReason               "fail_reason"
#define tcFieldInfo                     "info"
#define tcFieldModem                    "modem"
#define tcFieldWhen                     "when"
#define tcFieldSKU                      "sku"
#define tcFieldOrderingCode             "ordering_code"
#define tcFieldStation                  "station"
#define tcFieldOperator                 "operator"
#define tcFieldCheck                    "check"
#define tcFieldFirmwareOrg              "org"
#define tcFieldFirmwareProduct          "product"
#define tcFieldFirmwareVersion          "version"
#define tcFieldFirmwareMajor            "ver_major"
#define tcFieldFirmwareMinor            "ver_minor"
#define tcFieldFirmwarePatch            "ver_patch"
#define tcFieldFirmwareBuilt            "built"

// post.c
err_t postSelfTest(bool performHardwareTest, J *tc);
J *postGetTestCert(void);
J *postGetCachedTestCert(void);

// config.c
#define cstrsz 48
extern bool configReceivedHello;
extern char configModemVersion[cstrsz];
extern char configModemId[cstrsz];
extern char configPolicy[cstrsz];
extern char configSku[cstrsz];
extern char configOc[cstrsz];
extern char configApn[cstrsz];
extern char configBand[cstrsz];
extern char configChannel[cstrsz];
extern uint16_t configMtu;
void configSetDefaults(void);
void configSet(J *body);

// From Notecard CID definitions in notehub-defs.h
#define CID_NONE                    0
#define CID_RANDOM                  1

// modem.c
extern char workDetailedStatus[64];
err_t modemInit(void);
typedef err_t (*modemWorker) (J *workBody, uint8_t *workPayload, uint32_t workPayloadLen);
void modemTask(void *params);
err_t modemSend(arrayString *retResults, char *format, ...);
err_t modemEnqueueWork(const char *status, modemWorker worker, J *workBody, char *workPayloadB64);
err_t modemRemoveWork(modemWorker worker);
err_t modemRequireResults(arrayString *results, err_t err, uint32_t numResults, char *errType);
int modemResult(arrayString *results, char *prefix);
void modemUrcRemove(char *urc);
bool modemUrcGet(char *urc, char *buf, uint32_t buflen);
bool modemUrc(char *urc, bool remove);
bool modemHasReceivedLine(char *line);
void modemUrcShow(void);
bool modemIsConnected(void);
void modemSetConnected(bool yesOrNo);
err_t modemReportStatus(void);
void modemProcessSerialIncoming(void);
bool modemInfoNeeded(void);
bool modemWorkExists(modemWorker worker);

// power.c
#define POWER_INIT      0x00000001
#define POWER_DATA      0x00000002
#define POWER_GPS       0x00000004
#define POWER_MODEM_DFU 0x00000008
extern uint32_t powerNeeds;
extern bool gpsPoweredOn;
extern bool modemPoweredOn;
extern bool modemIsReady;
err_t powerOn(uint32_t reason);
void powerOff(uint32_t reason);

// work.c
err_t workModemConnect(J *body, uint8_t *payload, uint32_t payloadLen);
err_t workModemDisconnect(J *body, uint8_t *payload, uint32_t payloadLen);
err_t workModemUplink(J *body, uint8_t *payload, uint32_t payloadLen);
err_t workModemDownlink(J *hexData, uint8_t *payload, uint32_t payloadLen);
err_t workModemSend(J *jcmd, uint8_t *unused, uint32_t unusedLen);
err_t workEnableGps(J *junused, uint8_t *unused, uint32_t unusedLen);
err_t workDisableGps(J *junused, uint8_t *unused, uint32_t unusedLen);

// serial.c
extern bool serialActive;
void serialInit(uint32_t serialTaskID);
void serialPoll(void);
bool serialIsDebugPort(UART_HandleTypeDef *huart);
bool serialLock(UART_HandleTypeDef *huart, uint8_t **retData, uint32_t *retDataLen, bool *retDiagAllowed);
void serialUnlock(UART_HandleTypeDef *huart, bool reset);
void serialOutputString(UART_HandleTypeDef *huart, char *buf);
void serialOutput(UART_HandleTypeDef *huart, uint8_t *buf, uint32_t buflen);
void serialOutputLn(UART_HandleTypeDef *huart, uint8_t *buf, uint32_t buflen);
void serialOutputObject(UART_HandleTypeDef *huart, J *msg);
void serialSendMessageToNotecard(J *msg);
J *serialCreateMessage(const char *msgType, char *status, J *body, uint8_t *payload, uint32_t payloadLen);
void serialSendLineToModem(char *text);
void serialSendLineToNotecard(char *msg);
void serialResetUSB(void);

// maintask.c
void mainTask(void *params);

// button.c
void buttonPressISR(bool pressed);

// led.c
void ledRestartSignal();
void ledBusy(bool enable);

// reqtask.c
#define rdtNone             0
#define rdtRestart          1
#define rdtBootloader       2
void reqTask(void *params);
void reqButtonPressedISR(void);

// req.c
err_t reqProcess(bool debugPort, uint8_t *reqJSON, uint32_t reqJSONLen, bool diagAllowed, J **retRsp);

// diag.c
err_t diagProcess(char *diagCommand);

// montask.c
void monTask(void *params);

// monitor.c
uint32_t monitor(void);
void monitorReceivedHello(void);
bool monitorHaveReceivedHello(void);

// gps.c
extern double gpsLastHDOP;
extern double gpsLastLat;
extern double gpsLastLon;
extern int64_t gpsLastLatLonMs;
extern uint32_t gpsLastLatLonSecs;
extern uint32_t gpsLastTimeSecs;
bool gpsReceivedLine(char *line);

// Errors
#define ERR_IO "{io}"
#define ERR_TIMEOUT "{timeout}"
#define ERR_MODEM_COMMAND "{modem}"
