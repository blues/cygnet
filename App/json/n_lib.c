
#include "global.h"
#include "n_lib.h"

void *_Malloc(size_t len)
{
    void *p;
    if (memAlloc((uint32_t)len, &p) != errNone) {
        return NULL;
    }
    return p;
}

void _Free(void *p)
{
    if (p != NULL) {
        memFree(p);
    }
}

