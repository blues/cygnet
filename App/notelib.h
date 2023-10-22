
#include "board.h"
#include "app.h"
#include "gmutex.h"
#include "json.h"
#include "lora_conf.h"

#pragma once

// Limits
#define maxProductUID       96
#define maxSN               150
#define maxNotefileID       40
#define maxDeviceUID        48
#define maxDeviceSKU        64
#define maxDeviceOC         64
#define maxNoteID           32

// Configuration data
typedef struct {
#define configFieldOpMode               "op"
    uint32_t opMode;
#define configFieldProduct              "prod"
    char productUID[maxProductUID];
#define configFieldSN                   "sn"
    char deviceSN[maxSN];
#define configFieldInbound              "in"
    uint32_t inboundMins;
#define configFieldOutbound             "out"
    uint32_t outboundMins;
#define configFieldTraceLevel           "tr"
    uint32_t traceLevel;
#define configFieldTraceRequests        "treq"
    uint32_t traceRequests;
#define configFieldNotefileTemplates    "tpl"
    arrayMap *notefileTemplates;
#define configFieldLat                  "lat"
    double lat;
#define configFieldLon                  "lon"
    double lon;
#define configFieldLTime                "ltime"
    double ltime;
#define configFieldNeoMonitor           "neo"
    bool neomon;
#define configFieldNeoBrightness        "neob"
    bool neobrt;
#define configFieldGPIO                 "gpio"
    bool gpio;
#define configFieldGPIOUsage            "gpiou"
    char gpioUsage[64];                 // "input-pulldown,input-pulldown,input-pulldown,input-pulldown"
} config_t;
extern config_t config;

// Permanent configuration data
typedef struct {
#define configpFieldDevEui              "deveui"
    char devEui[24];
#define configpFieldAppEui              "appeui"
    char appEui[24];
#define configpFieldAppKey              "appkey"
    char appKey[48];
#define configpFieldI2CAddress          "i2c"
    uint32_t i2cAddress;
#define configpFieldTestCert            "tcert"
    J *tcert;
#define configpFieldTraceRequests       "tr"
    bool traceRequests;
} configp_t;
extern configp_t configp;

// Test certificate
#define tcFieldDeviceUID                "device"
#define tcFieldDefaultProductUID        "default_product"
#define tcFieldError                    "err"
#define tcFieldStatus                   "status"
#define tcFieldTests                    "tests"
#define tcFieldFailTest                 "fail_test"
#define tcFieldFailReason               "fail_reason"
#define tcFieldInfo                     "info"
#define tcFieldBoardVersion             "board"
#define tcFieldBoardType                "board_type"
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
#define tcFieldFirmwareBuild            "ver_build"
#define tcFieldFirmwareBuilt            "built"
#define tcFieldDevEui                   "deveui"
#define tcFieldAppEui                   "appeui"
#define tcFieldAppKey                   "appkey"
#define tcFieldFreq                     "freqplan"
#define tcFieldLWVersion                "lorawan"
#define tcFieldPHVersion                "regional"

// What we keep in template array maps
typedef struct {
    uint8_t port;
    bool confirmed;
    bool outbound;
    bool inbound;
    uint16_t crc;
} templateDescriptor;

// For defining notefile templates below
#define QUOTE               "\""
#define QSTR                TSTRINGV
#define QN(name,type)      QUOTE name QUOTE ":" STRINGIFY(type)
#define QS(name,type)      QUOTE name QUOTE ":" QUOTE type QUOTE
// System notefile ports
#define LPORT_SYS_NOTEFILE_CONFIG       (LPORT_SYS_NOTEFILE_FIRST+0)
#define sysNotefileConfig               "_config.qo"        // pseudo-notefile used for notecard configuration (see config.c)
#define tConfigProduct                  "p"
#define tConfigSn                       "s"
#define tConfigVersion                  "v"
#define tConfigSku                      "k"
#define tConfigOc                       "o"
#define tConfig "{" QS(tConfigProduct,QSTR) "," QS(tConfigSn,QSTR) "," QN(tConfigVersion,TUINT32) "," QS(tConfigSku,QSTR) "," QS(tConfigOc,QSTR) "}"

#define LPORT_SYS_NOTEFILE_TEMPLATE     (LPORT_SYS_NOTEFILE_FIRST+1)
#define sysNotefileTemplate             "_template.dbo"      // pseudo-notefile used to hold user notefile templates
#define tTemplateNotefile               "n"
#define tTemplatePort                   "p"
#define tTemplateText                   "t"
#define tTemplate "{" QN(tTemplatePort,TUINT8) "," QS(tTemplateNotefile,QSTR) "," QS(tTemplateText,QSTR) "}"

#define LPORT_SYS_NOTEFILE_ENV          (LPORT_SYS_NOTEFILE_FIRST+2)
#define tSysenvSn                       "_sn"               // TSTRING (see config.c)
#define tSysenvSnDefault                "_sn_default"       // TSTRING (see config.c)
#define tSysenvRestart                  "_restart"          // TUINT32 (see config.c)
#define sysNotefileEnv                  "_env.dbi"          // user defines template using env.template

// Local-only notefiles
#define LPORT_LOCAL                     0                   // never transmitted or received
#define sysNotefileUserEnvDefault       "_env.dbx"          // notefile used to hold user env var defaults

// Operating mode
#define OPMODE_PERIODIC                     0
#define OPMODE_OFF                          1
#define OPMODE_CONTINUOUS                   2
#define OPMODE_MINIMUM                      3

// When port number and confirmed are marshaled, it's the port number followed by this
// separator.  If the port number is negative, it's confirmed.
#define configPortNumSep ':'
#define hubSynclogNotefile              "_synclog.qi"

// Synclog defs
#define SLMAJOR         0
#define SLMINOR         1
#define SLDETAIL        2
#define SLPROG          3

#define SSWORK          "work"
#define SSWIRELESS      "wireless"
#define SSNETWORK       "network"
#define SSNOTEHUB       "notehub"
#define SSPROJECT       "project"
#define SSTRANSACTION   "request"

#define SSENUM SSWORK ":Sync Work" \
    "," SSWIRELESS ":Wireless Access" \
    "," SSNETWORK ":Network Access" \
    "," SSNOTEHUB ":Notehub Access" \
    "," SSPROJECT ":Project Access" \
    "," SSTRANSACTION ":Request" \
    ""

// Data structure for a sync log entry.  Note that there are two arrays kept in parallel: the array of these
// data structures, and a string array containing the log.
typedef struct {
#define slFieldTime                 "time"
    int64_t Time;
#define slFieldBootMs               "sequence"
    int64_t BootMs;
#define slFieldDetailLevel          "level"
    uint32_t DetailLevel;
#define slFieldSubsystem            "subsystem"
    char Subsystem[10];
#define slFieldText                 "text"
    char *Text;
    uint32_t TextLen;
} synclogBody;

// maintask.c
void mainTask(void *params);

// attn.c
#define ATTN_RESET          0x00000001
#define ATTN_MODIFIED       0x00000002
#define ATTN_USB            0x00000020
#define ATTN_AUX_GPIO       0x00000800
#define ATTN_ENV            0x00001000
bool attnIsArmed(void);
void attnSignal(uint32_t event);
void attnSignalFromISR(uint32_t event);
uint32_t attnMonitoring(void);
void attnClearReset(void);
err_t attnFileModified(char *notefileID);
err_t attnEnableModified(J *filesArray);
err_t attnGetEvents(J **filesModified, bool showMonitored, int32_t *seconds);
err_t attnDisableAll();
err_t attnEnableUSB(void);
err_t attnDisableUSB(void);
err_t attnEnableENV(void);
err_t attnDisableENV(void);
err_t attnEnableAuxGPIO(void);
err_t attnDisableAuxGPIO(void);
err_t attnDisableModified(void);
err_t attnReset(int32_t timeoutSecs, bool performSleep, bool disablePins);
uint32_t attnPoll(void);
void attnDisarm(void);
void attnArm(void);
bool attnDisable(void);
void attnEnable(void);
void attnResetHost(void);

// sync.c
extern int64_t syncBeganMs;
extern int64_t syncCompletedMs;
extern int64_t syncRequestedMs;
extern const char *syncError;
void syncNow(void);
void syncPing(void);
void syncTask(void *params);
void syncTxCompleted(uint8_t port, bool confirm, bool confirmAckReceived);
void syncRxCompleted(uint8_t port, uint8_t *buf, uint32_t buflen);
void syncInbound(bool tryHard);
void syncRejoinRequired(void);

// reqtask.c
#define rdtNone             0
#define rdtRestart          1
#define rdtBootloader       2
#define rdtRestore          3
extern uint32_t reqDeferredWork;
void reqTask(void *params);
void reqButtonPressedISR(void);

// req.c
err_t reqProcess(bool debugPort, uint8_t *reqJSON, uint32_t reqJSONLen, bool diagAllowed, uint8_t **rspJSON, uint32_t *rspJSONLen);

// diag.c
err_t diagProcess(char *cmd);

// inbound.c
void inboundNotefile(uint8_t port, uint8_t *data, uint32_t datalen);

// notefile.c
bool notefileIDIsReserved(char *notefileID);
err_t notefileAttributesFromID(char *notefileID, bool *optIsQueue, bool *optSyncToHub, bool *optSyncFromHub, bool *optSecure);
err_t notefileAddNote(char *notefileID, bool isQueue, char *noteID, bool update, J *template, J *body, uint8_t *payload, uint32_t payloadLen, uint8_t port, bool confirmed, bool inbound, bool outbound, uint32_t *retTotal, uint32_t *retBytes);
err_t notefileGetNote(char *notefileID, char *noteID, bool delete, J **retBody, J **retPayload);

// binary.c
err_t binaryFromNote(J *template, char *noteID, J *body, uint8_t *payload, uint32_t payloadLen, uint8_t **retBinary, uint32_t *retBinaryLen);
err_t binaryToNote(J *rootTemplate, uint8_t *binary, uint32_t binaryLen, J **retBody, char *retNoteID, uint32_t retNoteIDLen, uint8_t **retPayload, uint32_t *retPayloadLen);
err_t binaryValidateTemplate(J *body);

// page.c
#define PAGE_TYPE_DIRECTORY             0       // Directory page (singleton)
#define PAGE_TYPE_FREE                  1       // Free page (many)
#define PAGE_TYPE_UNUSABLE              2       // Don't try to write this (many)
#define PAGE_TYPE_CONFIG                3       // Config (singleton)
#define PAGE_TYPE_NAMED                 4       // Named page
#define PAGE_TYPE_TEMPLATE              5       // Templates (singleton)
#define PAGE_TYPE_XQ_FILLING            6       // Transmit queue (singleton)
#define PAGE_TYPE_XQ_FULL               7       // Transmit queue (many)
#define PAGE_TYPE_LORAWAN_NVM           8       // LORAWAN parameters (singleton)
typedef uint8_t pagetype_t;                     // PTYPES is actually just a nybble
#define PAGE_MRU        -1
#define PAGE_ACTIVE     PAGE_MRU    // for singletons
#define PAGE_LRU        0
typedef struct {
    uint16_t pagenum;
    pagetype_t pagetype;
    uint16_t pageage;
} pagedesc_t;
uint32_t pageCount(pagetype_t pt);
bool pageLock(void);
void pageUnlock(void);
bool pageRead(pagetype_t pt, char *name, int whichPage);
void pageInvalidate(bool);
bool pageWrite(pagetype_t pt, char *name);
void *pageBuffer(void);
uint32_t pageBufferLen(void);
void pageFactoryReset(void);
J *pageReadJSON(pagetype_t pt, char *name);
bool pageWriteJSON(pagetype_t pt, char *name, J *j);
bool pageDelete(pagetype_t pt, char *name);
array *pageList(pagetype_t pt);
bool pageListRead(pagedesc_t *desc);

// config.c
void configGetDeviceUID(char *buf, uint32_t buflen);
void configLoad(void);
void configUpdate(void);
void configpLoad(void);
void configpUpdate(void);
void configUpdateLorawan(void);
void configEnableLorawan(void);
bool configTemplateGetNotefileID(int8_t port, char *buf, uint32_t buflen);
void configAddSysEnvToTemplateBody(J *body);
J *configGetSystemTemplateObject(char *templateName);
err_t configTemplateDelete(char *notefileID);
J *configTemplateGetNotefiles(bool includeReserved);
err_t configTemplateIsPortUsed(char *notefileID, uint8_t port, bool inbound, bool outbound, char *usedByNotefileID);
err_t configTemplateSet(char *notefileID, uint8_t port, bool confirmed, bool inbound, bool outbound, char *value, bool trace);
bool configTemplateGetLock(char *notefileID, char **template, uint8_t *port, bool *confirmed, bool *inbound, bool *outbound);
void configTemplateUnlock(void);
err_t configTemplateGetObject(char *notefileID, J **retTemplate, uint8_t *retPort, bool *retConfirmed, bool *retInbound, bool *retOutbound);
int8_t configTemplateGetPort(char *notefileID);
void configInitialUplinksNeeded(void);
bool configInitialUplinksInProgress(void);
int configToCompositePort(bool confirm, uint8_t port);
void configFromCompositePort(int cport, bool *confirm, uint8_t *port);
void configGetHardwareDeviceEui(bool uppercase, char *buf, uint32_t buflen);
char *configProductUID(void);
bool configTestCertificate(void);
void configDownlinkCompleted(uint8_t port, uint8_t *data, uint32_t datalen);

// post.c
err_t postSelfTest(bool performHardwareTest, J *tc);

// xq.c
err_t xqPut(uint64_t uid, bool confirmed, uint8_t port, uint8_t *data, uint32_t datalen, bool freeData);
bool xqGet(uint64_t *uid, bool *confirmed, uint8_t *port, uint8_t **data, uint32_t *datalen);
uint32_t xqLength(void);
bool xqDelete(uint64_t uid);
uint32_t xqpEnum(bool dump, J *info, uint32_t *retOutboundQueueNoteCount);

// xqp.c
#define xqpPutString(c,p,b) xqpPut(c,p,(uint8_t *)(b),strlen(b))
err_t xqpPut(bool confirmed, uint8_t port, uint8_t *binary, uint32_t binarylen);
bool xqpMigrate(uint32_t maxEntriesToMigrate);

// env.c
bool envVarIsReserved(char *name);
uint32_t envModifiedTime(void);
void envUpdate(void);
J *envGetString(char *key);
int envGetInt(char *name);
void envGetAll(J *rsp, J *filterArray);
void envLoadFromNoteToNotefile(J *body);

// smazz.c
int smazzEncode(char *in, int inlen, char *out, int outlen);
int smazzDecode(char *in, int inlen, char *out, int outlen);
void smazzTest(void);

// button.c
void buttonPressISR(bool pressed);

// led.c
#define FLED_REQ_ACTIVITY       0
#define FLED_BUTTON_CONFIRM     1
#define FLED_NET_CONNECTING     2
#define FLED_NET_ACTIVITY       3
#define FLED_NET_PENALTY        4
#define FLED_COUNT              5
void ledRestartSignal(void);
void ledBusy(bool enable);
bool ledNetErrorPending(void);
void ledInit(void);
void ledEnable(int fled, bool on);
void ledNetError(bool on);
void ledNetPenalty(bool enable);
uint32_t ledNetPenaltyPending(void);

// serial.c
void serialResetUSB(void);

// gstate.c
err_t gstateRefresh(void);
J *gstateGet(void);
void gstateAuxInterrupt(uint16_t pinIndex);

// synclog.c
J *synclogDequeue(bool delete);
void synclogf(uint32_t detailLevel, const char *subsystem, const char *progid, const char *format, ...);
void synclogTrace(bool enabled, uint32_t level);
bool synclogTraceEnabled(void);

// errors
#define ERR_IO "{io}"
#define ERR_NOTEFILE_NOEXIST "{notefile-noexist}"
#define ERR_NOTE_NOEXIST "{note-noexist}"
#define ERR_NOTE_EXISTS "{note-exists}"
#define ERR_ENV_NOT_MODIFIED "{env-not-modified}"
#define ERR_INCOMPATIBLE "{incompatible}"
#define ERR_SYNTAX "{syntax}"
#define ERR_IDLE "{idle}"
#define ERR_NETWORK "{network}"
#define ERR_EXTENDED_NETWORK_FAILURE "{extended-network-failure}"
#define ERR_TRANSPORT_UNAVAILABLE "{unavailable}"
#define ERR_TRANSPORT_CONNECTED "{connected}"
#define ERR_TRANSPORT_DISCONNECTED "{disconnected}"
#define ERR_TRANSPORT_CONNECTING "{connecting}"
#define ERR_TRANSPORT_CONNECT_FAILURE "{connect-failure}"
#define ERR_TRANSPORT_WAIT_SERVICE "{wait-service}"
#define ERR_SYNTAX "{syntax}"
#define ERR_CRC "{crc}"
#define ERR_PRODUCT_UID "{product-noexist}"

#define STATUS_SYNC_IN_PROGRESS "{sync}"
#define STATUS_SYNC_COMPLETED "{sync-completed}"
#define STATUS_SYNC_ERROR "{sync-error}"
#define STATUS_NETWORK_WAIT "{network-error-wait}"
#define STATUS_JOINING "{joining-network}"
#define STATUS_SYNC_ERROR "{sync-error}"
#define STATUS_SYNC_END "{sync-end}"
