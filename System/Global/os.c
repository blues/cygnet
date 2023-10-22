
#include "buildnum.h"
#include "main.h"
#include "global.h"
#include "product.h"

// See if the debugger is currently active
bool osDebugging(void)
{
    return MX_DBG_Active();
}

// Get the build number
uint32_t osBuildNum(void)
{
    return BUILDNUMBER;
}

// Return configuration JSON object
char *osBuildConfig()
{
    char *configStr = PRODUCT_CONFIG;
    int signatureLen = sizeof(PRODUCT_CONFIG_SIGNATURE)-1;
    return configStr+signatureLen;
}

// See if USB is currently active
bool osUsbDetected(void)
{
#ifdef USB_DETECT_Pin
    return (HAL_GPIO_ReadPin(USB_DETECT_GPIO_Port, USB_DETECT_Pin) == GPIO_PIN_SET);
#else
    return true;
#endif
}

// Case-insensitive ASCII comparison
bool streqlCI(const char *a, const char *b)
{
    return memeqlCI((char *)a, (char *)b, strlen(a)+1);
}

// Counted ASCII comparison
bool memeqlCI(void *av, void *bv, int len)
{
    register char *a = (char *) av;
    register char *b = (char *) bv;
    for (register int i=0; i<len; i++) {
        register char c1 = *a++;
        register char c2 = *b++;
        if (c1 >= 'A' && c1 <= 'Z') {
            c1 += 'a'-'A';
        }
        if (c2 >= 'A' && c2 <= 'Z') {
            c2 += 'a'-'A';
        }
        if (c1 != c2) {
            return false;
        }
    }
    return true;
}

// Convert number to an 8-byte null-terminated hex string
void htoa32(uint32_t n, char *p)
{
    int i;
    for (i=0; i<8; i++) {
        uint32_t nibble = (n >> 28) & 0xff;
        n = n << 4;
        if (nibble >= 10) {
            *p++ = 'A' + (nibble-10);
        } else {
            *p++ = '0' + nibble;
        }
    }
    *p = '\0';
}

// Convert number to a 4-byte null-terminated hex string
void htoa16(uint16_t n, unsigned char *p)
{
    int i;
    for (i=0; i<4; i++) {
        uint16_t nibble = (n >> 12) & 0xff;
        n = n << 4;
        if (nibble >= 10) {
            *p++ = 'A' + (nibble-10);
        } else {
            *p++ = '0' + nibble;
        }
    }
    *p = '\0';
}

// Convert a hex string to a number
uint64_t atoh(char *p, int maxlen)
{
    uint64_t n = 0;
    char *ep = p+maxlen;
    while (p < ep) {
        char ch = *p++;
        bool digit = (ch >= '0' && ch <= '9');
        bool lcase = (ch >= 'a' && ch <= 'f');
        bool space = (ch == ' ');
        bool ucase = (ch >= 'A' && ch <= 'F');
        if (!digit && !lcase && !space && !ucase) {
            break;
        }
        n *= 16;
        if (digit) {
            n += ch - '0';
        } else if (lcase) {
            n += 10 + (ch - 'a');
        } else if (ucase) {
            n += 10 + (ch - 'A');
        }
    }
    return (n);
}

// Convert hex string to hex binary
void stoh(char *src, uint8_t *dst, uint32_t dstlen)
{
    uint16_t bytes = GMIN(strlen(src)/2,dstlen);
    memset(dst, 0, dstlen);
    for (int i=0; i<bytes; i++) {
        char hex[3];
        hex[0] = *src++;
        hex[1] = *src++;
        hex[2] = '\0';
        *dst++ = (uint8_t) atoh(hex, 2);
    }
}

