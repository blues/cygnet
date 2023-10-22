// Copyright 2019 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "global.h"

// Type-casting methods
uint32_t wordFromFloat(float x)
{
    return *(uint32_t*)&x;
}
float floatFromWord(uint32_t x)
{
    return *(float*)&x;
}

// IEEE-754 16-bit floating-point format (without infinity):
// 1-5-10, exp-15, +-131008.0, +-6.1035156E-5, +-5.9604645E-8, 3.311 digits
float16_t float16FromFloat32(float x)
{
    // round-to-nearest-even: add last bit after truncated mantissa
    uint32_t b = wordFromFloat(x) + 0x00001000;
    // exponent
    uint32_t e = (b & 0x7F800000) >> 23;
    // mantissa; in line below: 0x007FF000 = 0x00800000-0x00001000 = decimal indicator flag - initial rounding
    uint32_t m = b & 0x007FFFFF;
    // sign : normalized : denormalized : saturate
    return (b&0x80000000)>>16 | (e>112)*((((e-112)<<10)&0x7C00)|m>>13) | ((e<113)&(e>101))*((((0x007FF000+m)>>(125-e))+1)>>1) | (e>143)*0x7FFF;
}

// IEEE-754 16-bit floating-point format (without infinity):
// 1-5-10, exp-15, +-131008.0, +-6.1035156E-5, +-5.9604645E-8, 3.311 digits
float float16ToFloat32(float16_t x)
{
    // exponent
    uint32_t e = (x&0x7C00)>>10;
    // mantissa
    uint32_t m = (x&0x03FF)<<13;
    // evil log2 bit hack to count leading zeros in denormalized format
    uint32_t v = wordFromFloat((float)m)>>23;
    // sign : normalized : denormalized
    return floatFromWord((x&0x8000u)<<16 | (e!=0)*((e+112)<<23|m) | ((e==0)&(m!=0))*((v-37)<<23|((m<<(150-v))&0x007FE000u)));
}

