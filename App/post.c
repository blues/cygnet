
#include "app.h"
#include "flash_if.h"

// The in-mem test cert
STATIC J *testCert = NULL;

// Valid SKUs
typedef struct {
    const char *sku;
} sku_t;
const sku_t skus[] = {
    {.sku = "NTN-SKY1" },
};

// Self-test, writing the test certificate upon completion using the supplied template
err_t postSelfTest(bool performHardwareTest, J *tc)
{

    // Validate the cert
    if (tc == NULL) {
        return errF("no test cert template");
    }

    // If the SKU is present, map it to the frequency field
    bool validSKU = false;
    char *sku = JGetString(tc, tcFieldSKU);
    if (sku[0] == '\0') {
        return errF("sku is required");
    }
    for (int i=0; i<sizeof(skus)/sizeof(skus[0]); i++) {
        if (streql(skus[i].sku, sku)) {
            validSKU = true;
            break;
        }
    }
    if (!validSKU) {
        return errF("unrecognized sku: %s", sku);
    }

    // Overwrite the test cert so that it is re-generated
    configDeleteTestCert();
    if (testCert != NULL) {
        memFree(testCert);
        testCert = NULL;
    }

    // Wait for modem info to be loaded by the monitor task
    int64_t expireMs = timerMs() + (60 * ms1Sec);
    while (modemInfoNeeded()) {
        if (timerMs() >= expireMs) {
            return errF("can't obtain modem information");
        }
        timerMsSleep(750);
    }

    // Clear out thing that we are going to set, just in case called with an object we're updating
    JDeleteItemFromObject(tc, tcFieldError);
    JDeleteItemFromObject(tc, tcFieldStatus);
    JDeleteItemFromObject(tc, tcFieldTests);
    JDeleteItemFromObject(tc, tcFieldFailTest);
    JDeleteItemFromObject(tc, tcFieldFailReason);
    JDeleteItemFromObject(tc, tcFieldCheck);
    JDeleteItemFromObject(tc, tcFieldModem);
    JDeleteItemFromObject(tc, tcFieldDeviceUID);

    JAddStringToObject(tc, tcFieldDeviceUID, configModemId);
    JAddStringToObject(tc, tcFieldModem, configModemVersion);

    JAddIntToObject(tc, tcFieldCid, STARNOTE_CID_TYPE);
    JAddStringToObject(tc, tcFieldPolicy, STARNOTE_DEFAULT_POLICY);
    JAddIntToObject(tc, tcFieldMtu, STARNOTE_DEFAULT_MTU);
    JAddStringToObject(tc, tcFieldApn, STARNOTE_DEFAULT_APN);
    JAddStringToObject(tc, tcFieldBand, STARNOTE_DEFAULT_BAND);
    JAddStringToObject(tc, tcFieldChannel, STARNOTE_DEFAULT_CHANNEL);

    JDeleteItemFromObject(tc, tcFieldFirmwareOrg);
    JAddStringToObject(tc, tcFieldFirmwareOrg, PRODUCT_MANUFACTURER);
    JDeleteItemFromObject(tc, tcFieldFirmwareProduct);
    JAddStringToObject(tc, tcFieldFirmwareProduct, PRODUCT_DISPLAY_NAME);
    JDeleteItemFromObject(tc, tcFieldFirmwareVersion);
    char *ver = PRODUCT_PROGRAMMATIC_NAME "-" STRINGIFY(PRODUCT_MAJOR_VERSION) "." STRINGIFY(PRODUCT_MINOR_VERSION) "." STRINGIFY(PRODUCT_PATCH_VERSION);
    JAddStringToObject(tc, tcFieldFirmwareVersion, ver);
    JDeleteItemFromObject(tc, tcFieldFirmwareMajor);
    JAddIntToObject(tc, tcFieldFirmwareMajor, PRODUCT_MAJOR_VERSION);
    JDeleteItemFromObject(tc, tcFieldFirmwareMinor);
    JAddIntToObject(tc, tcFieldFirmwareMinor, PRODUCT_MINOR_VERSION);
    JDeleteItemFromObject(tc, tcFieldFirmwarePatch);
    JAddIntToObject(tc, tcFieldFirmwarePatch, PRODUCT_PATCH_VERSION);
    JDeleteItemFromObject(tc, tcFieldFirmwareBuilt);
    JAddStringToObject(tc, tcFieldFirmwareBuilt, __DATE__ " " __TIME__);

    // Create a flash I/O buf and set it as context for the flash subsystem
    void *iobuf;
    err_t err = memAlloc(FLASH_PAGE_SIZE, &iobuf);
    if (err) {
        return err;
    }
    FLASH_IF_Init(iobuf);

    // Print the test cert to a buffer
    char *json = JPrintUnformattedOmitEmpty(tc);
    uint32_t jsonLen = strlen(json);
    if (jsonLen+1 > TESTCERT_FLASH_LEN) {
        return errF("test cert too large");
    }

    // Write it, noting that FLASH_IF_Write requires 64-bit alignment
    uint32_t lengthToWrite = jsonLen+1;
    lengthToWrite = ((lengthToWrite+7) & ~7);
    err = errNone;
    if (FLASH_IF_OK != FLASH_IF_Write(TESTCERT_FLASH_ADDRESS, json, lengthToWrite)) {
        err = errF("flash write error");
    }

    // Release the JSON
    memFree(json);

    // Release the I/O buf
    FLASH_IF_DeInit();
    memFree(iobuf);

    // If error, hang
    if (err) {
        debugf("*** FAILURE: %s ***\n", errString(err));
        ledBusy(true);
        while (true) ;
    }

    // Swap the cached version
    testCert = postGetTestCert();

    // Done
    return errNone;

}

// Retrieve the test certificate
J *postGetTestCert()
{

    // See if there's a null terminator in the test cert page.  Note that
    // there is a bug in the IAR library such that a memchr that includes the very
    // last byte of RAM will fault:
    //   __iar_Memchr
    //   Hardfault exception.
    //   The processor has escalated a configurable-priority exception to HardFault.
    //   A precise data access error has occurred (CFSR.PRECISERR, BFAR)
    //   At data address 0x8040000.
    // And so we test the last byte manually.
    if (((char *)TESTCERT_FLASH_ADDRESS)[0] != '{') {
        return NULL;
    }
    char *pnull = memchr(TESTCERT_FLASH_ADDRESS, 0, TESTCERT_FLASH_LEN-1);
    if (pnull == NULL) {
        if (((uint8_t *)TESTCERT_FLASH_ADDRESS)[TESTCERT_FLASH_LEN-1] != '\0') {
            return NULL;
        }
    }

    // Parse it, directly from flash in high mem
    return JParse(TESTCERT_FLASH_ADDRESS);

}

// Retrieve the test certificate
J *postGetCachedTestCert()
{

    // If it's loaded, return it
    if (testCert != NULL) {
        return testCert;
    }

    // Read it and cache it
    testCert = postGetTestCert();

    // Return it
    return testCert;

}

