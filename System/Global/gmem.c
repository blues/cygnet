// Copyright 2017 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "FreeRTOS.h"
#include "global.h"

// Remember the count of objects allocated
long memObjects = 0;
long memFailures = 0;

// Currently free
uint32_t memCurrentlyFree(void)
{
    return (uint32_t) xPortGetFreeHeapSize();
}

// Alloc
err_t memAlloc(uint32_t length, void *ptr)
{
    void *p = pvPortMalloc((size_t)length);
    if (p == NULL) {
        memFailures++;
        return errF("cannot allocate %d bytes " ERR_MEM_ALLOC, length);
    }
    memObjects++;
    memset(p, 0, length);
    * (void **) ptr = p;
    return errNone;
}

// Free
void memFree(void *p)
{
    memObjects--;
    vPortFree(p);
}

// Realloc
err_t memRealloc(uint32_t fromLength, uint32_t toLength, void *ptr)
{
    uint8_t *new;
    err_t err = memAlloc(toLength, &new);
    if (err) {
        return err;
    }
    uint8_t *old = * (void **) ptr;
    memcpy(new, old, GMIN(toLength, fromLength));
    * (void **) ptr = new;
    memFree(old);
    return errNone;
}

// Duplicate an object
err_t memDup(void *pSrc, uint32_t srcLength, void *pCopy)
{
    uint8_t *copy;
    * (void **) pCopy = NULL;       // Guarantee that we always return null, even if error
    if (pSrc == NULL && srcLength != 0) {
        return errF("bad params");
    }
    if (pSrc == NULL) {
        return errNone;
    }
    err_t err = memAlloc(srcLength, &copy);
    if (err) {
        return err;
    }
    memcpy(copy, pSrc, srcLength);
    * (void **) pCopy = copy;
    return errNone;
}
