
#pragma once

#define FieldCmd        "cmd"
#define FieldStatus     "status"
#define FieldBody       "body"
#define FieldPayload    "payload"
#define FieldErr        "err"
#define FieldID         "id"

#define ReqHello        "ntn.hello"
#define ReqConnect      "ntn.connect"
#define ReqDisconnect   "ntn.disconnect"
#define ReqUplink       "ntn.uplink"
#define ReqDownlink     "ntn.downlink"
#define ReqStatus       "ntn.status"

#define NTN_INIT                "{ntn-initializing}"
#define NTN_CONNECTED           "{ntn-connected}"
#define NTN_POWER               "{ntn-power}"
#define NTN_CONNECTING          "{ntn-connecting}"
#define NTN_DISCONNECTING       "{ntn-disconnecting}"
#define NTN_UPLINKING           "{ntn-uplinking}"
#define NTN_DOWNLINKING         "{ntn-downlinking}"
#define NTN_IDLE                "{ntn-idle}"
#define NTN_NO_LOCATION         "{ntn-need-location}"
#define NTN_CONNECT_FAILURE     "{ntn-connect-failure}"
