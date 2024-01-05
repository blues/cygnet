
#include "app.h"
#include "flash_if.h"

bool configReceivedHello = false;
char configModemVersion[cstrsz] = {0};
char configModemId[cstrsz] = {0};
char configPolicy[cstrsz] = {0};
char configSku[cstrsz] = {0};
char configOc[cstrsz] = {0};
char configApn[cstrsz] = {0};
char configBand[cstrsz] = {0};
char configChannel[cstrsz] = {0};
uint16_t configMtu = 0;

// Reset the test cert so this is a "fresh" module
void configDeleteTestCert(void)
{
    configModemId[0] = '\0';
    configModemVersion[0] = '\0';
    uint8_t *iobuf;
    if (memAlloc(FLASH_PAGE_SIZE, &iobuf) == errNone) {
        FLASH_IF_Init(iobuf);
        uint8_t *p;
        if (memAlloc(sizeof(uint64_t), &p) == errNone) {
            FLASH_IF_Write(TESTCERT_FLASH_ADDRESS, p, sizeof(uint64_t));
            memFree(p);
        }
        memFree(iobuf);
    }
    taskGive(TASKID_MON);
}

// Set config vars
void configSetDefaults(void)
{
    strLcpy(configModemVersion, "");
    strLcpy(configModemId, "");
    strLcpy(configPolicy, STARNOTE_DEFAULT_POLICY);
    strLcpy(configSku, STARNOTE_DEFAULT_SKU);
    strLcpy(configOc, STARNOTE_DEFAULT_OC);
    strLcpy(configApn, STARNOTE_DEFAULT_APN);
    strLcpy(configBand, STARNOTE_DEFAULT_BAND);
    strLcpy(configChannel, STARNOTE_DEFAULT_CHANNEL);
    configMtu = STARNOTE_DEFAULT_MTU;
}

// Set config vars
void configSet(J *body)
{
    char *p;
    if (body == NULL) {
        return;
    }
    if (JIsPresent(body, STARNOTE_MTU_FIELD)) {
        int value = JGetInt(body, STARNOTE_MTU_FIELD);
        if (value < 0) {
            value = STARNOTE_DEFAULT_MTU;
        }
        configMtu = (uint16_t) value;
    }
    if (JIsPresent(body, STARNOTE_POLICY_FIELD)) {
        p = JGetString(body, STARNOTE_POLICY_FIELD);
        if (p[0] == '-') {
            p = STARNOTE_DEFAULT_POLICY;
        }
        strLcpy(configPolicy, p);
    }
    if (JIsPresent(body, STARNOTE_SKU_FIELD)) {
        p = JGetString(body, STARNOTE_SKU_FIELD);
        if (p[0] == '-') {
            p = STARNOTE_DEFAULT_SKU;
        }
        strLcpy(configSku, p);
    }
    if (JIsPresent(body, STARNOTE_OC_FIELD)) {
        p = JGetString(body, STARNOTE_OC_FIELD);
        if (p[0] == '-') {
            p = STARNOTE_DEFAULT_OC;
        }
        strLcpy(configOc, p);
    }
    if (JIsPresent(body, STARNOTE_APN_FIELD)) {
        p = JGetString(body, STARNOTE_APN_FIELD);
        if (p[0] == '-') {
            p = STARNOTE_DEFAULT_APN;
        }
        strLcpy(configApn, p);
    }
    if (JIsPresent(body, STARNOTE_BAND_FIELD)) {
        p = JGetString(body, STARNOTE_BAND_FIELD);
        if (p[0] == '-') {
            p = STARNOTE_DEFAULT_BAND;
        }
        strLcpy(configBand, p);
    }
    if (JIsPresent(body, STARNOTE_CHANNEL_FIELD)) {
        p = JGetString(body, STARNOTE_CHANNEL_FIELD);
        if (p[0] == '-') {
            p = STARNOTE_DEFAULT_CHANNEL;
        }
        strLcpy(configChannel, p);
    }
}
