
#include "global.h"

//  Processor-independent CRC-16 calculation using IBM polynomial
//  x^16 + x^15 + x^2 + 1 (0xA001)
//  Initial value: 0
uint16_t crc16(uint8_t const *data, size_t size)
{
    uint16_t crc = 0;
    while (size--) {
        crc ^= *data++;
        for (unsigned k = 0; k < 8; k++) {
            crc = crc & 1 ? (crc >> 1) ^ 0xA001 : crc >> 1;
        }
    }
    return crc;
}
