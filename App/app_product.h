
// Overrides for cygnet's product.h
#define PRODUCT_MANUFACTURER        "Blues Inc"
#define PRODUCT_DISPLAY_NAME        "Starnote"
#define PRODUCT_PROGRAMMATIC_NAME   "starnote"
#define PRODUCT_MAJOR_VERSION       1
#define PRODUCT_MINOR_VERSION       1
#define PRODUCT_PATCH_VERSION       1

#define PRODUCT_USB_VID             12452   // 0x30A4 Inca Roads LLC
#define PRODUCT_USB_PID             4       // Inca Roads LLC ID is 4 for the Starnote

#define STARNOTE_CID_TYPE           CID_NONE
#define STARNOTE_ID_SCHEME          "skylo:"
#define STARNOTE_DEFAULT_MTU        256
#define STARNOTE_DEFAULT_POLICY     "10TPM"
#define STARNOTE_DEFAULT_SKU        "NTN-SKYLO"
#define STARNOTE_DEFAULT_OC         ""
#define STARNOTE_DEFAULT_APN        "blues.demo"
#define STARNOTE_DEFAULT_BAND       "0"             // "23" is best for Manchester
#define STARNOTE_DEFAULT_CHANNEL    "0"             // "7697,2" is best for manchester

#define STARNOTE_MTU_FIELD          "mtu"
#define STARNOTE_POLICY_FIELD       "policy"
#define STARNOTE_SKU_FIELD          "sku"
#define STARNOTE_OC_FIELD           "oc"
#define STARNOTE_APN_FIELD          "apn"
#define STARNOTE_BAND_FIELD         "band"
#define STARNOTE_CHANNEL_FIELD      "channel"
#define STARNOTE_CID_FIELD          "cid"
#define STARNOTE_ID_FIELD           "id"
#define STARNOTE_MODEM_FIELD        "modem"
#define STARNOTE_CERT_FIELD			"cert"
