
#include "app.h"
#include "flash_if.h"

bool configReceivedHello = false;
char configModemVersion[cstrsz] = {0};
char configModemId[cstrsz] = {0};
char configPolicy[cstrsz] = {0};
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
void configGetDefaultsFromTestCert(void)
{

    // Set true defaults as though there were no test cert
    strLcpy(configModemVersion, "");
    strLcpy(configModemId, "");
    strLcpy(configPolicy, STARNOTE_DEFAULT_POLICY);
    strLcpy(configOc, STARNOTE_DEFAULT_OC);
    strLcpy(configApn, STARNOTE_DEFAULT_APN);
    strLcpy(configBand, STARNOTE_DEFAULT_BAND);
    strLcpy(configChannel, STARNOTE_DEFAULT_CHANNEL);
    configMtu = STARNOTE_DEFAULT_MTU;

    // Get the test cert
    J *tcert = postGetCachedTestCert();
    if (tcert == NULL) {
        return;
    }

    // Extract defaults from it
    if (JIsPresent(tcert, tcFieldDeviceUID)) {
        strLcpy(configModemId, JGetString(tcert, tcFieldDeviceUID));
    }
    if (JIsPresent(tcert, tcFieldModem)) {
        strLcpy(configModemVersion, JGetString(tcert, tcFieldModem));
    }
    if (JIsPresent(tcert, tcFieldPolicy)) {
        strLcpy(configPolicy, JGetString(tcert, tcFieldPolicy));
    }
    if (JIsPresent(tcert, tcFieldOrderingCode)) {
        strLcpy(configOc, JGetString(tcert, tcFieldOrderingCode));
    }
    if (JIsPresent(tcert, tcFieldApn)) {
        strLcpy(configApn, JGetString(tcert, tcFieldApn));
    }
    if (JIsPresent(tcert, tcFieldBand)) {
        strLcpy(configBand, JGetString(tcert, tcFieldBand));
    }
    if (JIsPresent(tcert, tcFieldChannel)) {
        strLcpy(configChannel, JGetString(tcert, tcFieldChannel));
    }
    if (JIsPresent(tcert, tcFieldMtu)) {
        configMtu = JGetInt(tcert, tcFieldMtu);
    }

}

// Set config vars
void configSet(J *body)
{
    char *p;
    if (body == NULL) {
        return;
    }
    if (JIsPresent(body, tcFieldMtu)) {
        int value = JGetInt(body, tcFieldMtu);
        if (value < 0) {
            value = STARNOTE_DEFAULT_MTU;
        }
        configMtu = (uint16_t) value;
    }
    if (JIsPresent(body, tcFieldPolicy)) {
        p = JGetString(body, tcFieldPolicy);
        if (p[0] == '-') {
            p = STARNOTE_DEFAULT_POLICY;
        }
        strLcpy(configPolicy, p);
    }
    if (JIsPresent(body, tcFieldApn)) {
        p = JGetString(body, tcFieldApn);
        if (p[0] == '-') {
            p = STARNOTE_DEFAULT_APN;
        }
        strLcpy(configApn, p);
    }
    if (JIsPresent(body, tcFieldBand)) {
        p = JGetString(body, tcFieldBand);
        if (p[0] == '-') {
            p = STARNOTE_DEFAULT_BAND;
        }
        strLcpy(configBand, p);
    }
    if (JIsPresent(body, tcFieldChannel)) {
        p = JGetString(body, tcFieldChannel);
        if (p[0] == '-') {
            p = STARNOTE_DEFAULT_CHANNEL;
        }
        strLcpy(configChannel, p);
    }
}
