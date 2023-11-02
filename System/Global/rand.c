// Copyright 2017 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "main.h"
#include "global.h"
#include "mutex.h"
#include "rng.h"

STATIC bool rand_initialized = false;
STATIC char rand_state[256] = {0};
char *rinitstate (unsigned int seed, char *arg_state, unsigned long n);
char *rsetstate (const char *arg_state);
long rrandom();

// Statics
STATIC mutex randMutex = {MTX_RAND, {0}};

// Method to generate a 32-bit TRNG, with on-demand initialization
uint32_t halGenerateRandomNumber()
{
    if (!(peripherals & PERIPHERAL_RNG)) {
        MX_RNG_Init();
    }
    return MX_RNG_Get();
}

// Get a pseudo-random 32-bit number in low-value cases where we KNOW that it's not worth turning on TRNG hardware
unsigned long int prandNumber()
{
    uint32_t number;
    mutexLock(&randMutex);
    if (!rand_initialized) {
        rand_initialized = true;
        uint32_t hardwareRandomSeed = halGenerateRandomNumber();
        rinitstate((unsigned int)hardwareRandomSeed, rand_state, sizeof(rand_state));
        rsetstate(rand_state);
    }
    number = (uint32_t) rrandom();
    mutexUnlock(&randMutex);
    return number;
}

// Get a random 32-bit number using a true random number generator where such a thing is available
unsigned long int randNumber()
{
    uint32_t number;
    mutexLock(&randMutex);
    number = halGenerateRandomNumber();
    mutexUnlock(&randMutex);
    return number;
}

// Get a buffer of random numbers using TRNG
unsigned long int randNumberBuffer(unsigned char* output, uint32_t sz)
{
    uint32_t i = 0;
    mutexLock(&randMutex);
    while (i < sz) {
        /* If not aligned or there is odd/remainder */
        if( (i + sizeof(uint32_t)) > sz ||
                ((uint32_t)&output[i] % sizeof(uint32_t)) != 0
          ) {
            /* Single byte at a time */
            output[i++] = (unsigned char)halGenerateRandomNumber();
        } else {
            /* Use native 8, 16, 32 or 64 copy instruction */
            *((uint32_t*)&output[i]) = halGenerateRandomNumber();
            i += sizeof(uint32_t);
        }
    }
    mutexUnlock(&randMutex);
    return sz;
}

// Generate a random number without modulo bias
uint32_t randEvenDistribution(uint32_t n)
{
    if (n == 0) {
        return randNumber();
    }
    if (n <= UINT8_MAX) {
        uint16_t limit = UINT8_MAX - UINT8_MAX % n;
        uint16_t rnd;
        do {
            rnd = (uint16_t) randNumber();
        } while (rnd >= limit);
        return rnd % n;
    }
    if (n <= UINT16_MAX) {
        uint16_t limit = UINT16_MAX - UINT16_MAX % n;
        uint16_t rnd;
        do {
            rnd = (uint16_t) randNumber();
        } while (rnd >= limit);
        return rnd % n;
    }
    uint32_t limit = UINT32_MAX - UINT32_MAX % n;
    uint32_t rnd;
    do {
        rnd = randNumber();
    } while (rnd >= limit);
    return rnd % n;
}
