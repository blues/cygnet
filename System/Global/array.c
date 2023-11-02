// Copyright 2017 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "global.h"
#include "mutex.h"

// Insert an entry into an array at the specified idnex
err_t arrayInsert(array *ctx, uint16_t i, void *data)
{

    // Out of range
    if (i > ctx->count) {
        return errF("array index out of range");
    }

    // Append
    if (i == ctx->count) {
        return (arrayAppend(ctx, data));;
    }

    // Compute data length for string arrays
    uint32_t datalen = ctx->size;
    if (datalen == 0) {
        if (data == NULL) {
            datalen = 1;
        } else {
            datalen = strlen(data) + 1;
        }
    }

    // Reallocate if we must grow
    if (datalen > (ctx->allocated - ctx->length)) {
        uint32_t alloclen = GMAX(ctx->chunksize, datalen);
        err_t err = memRealloc(ctx->allocated, ctx->allocated + alloclen, &ctx->address);
        if (err) {
            return err;
        }
        ctx->allocated = ctx->allocated + alloclen;
        ctx->cachedIndex = 0;
        ctx->cachedAddress = ctx->address;
    }

    // Copy everything upward, in a way that works for both fixed and string arrays
    uint8_t *from = arrayEntry(ctx, i);
    uint8_t *to = from + datalen;
    uint32_t len = ((uint8_t *)ctx->address+ctx->length) - from;
    memmove(to, from, len);

    // Copy the data
    if (data == NULL) {
        memset(from, 0, datalen);
    } else {
        memmove(from, data, datalen);
    }

    // Update the entries, in a way that works for both fixed and string arrays
    ctx->length += datalen;
    ctx->count++;

    // Reset the cache
    ctx->cachedIndex = 0;
    ctx->cachedAddress = ctx->address;

    // Done
    return errNone;
}

// Remove an entry from an array.  Note that this does NOT "free" the contents
// of the entry, so if that is what you intend you must do it manually.
void arrayRemove(array *ctx, uint16_t i)
{

    // Get the entry length
    uint8_t *to = arrayEntry(ctx, i);
    if (to == NULL) {
        return;
    }
    uint32_t size = ctx->size;
    if (ctx->size == 0) {
        size = strlen((char *)to)+1;
    }

    // If from comes back as NULL, it means that this is the last entry
    uint8_t *from = arrayEntry(ctx, i+1);
    if (from != NULL) {
        uint32_t len = ((uint8_t *)ctx->address+ctx->length) - from;
        memmove(to, from, len);
    }

    // Update the length and count
    ctx->length -= size;
    ctx->count--;

    // Reset the cache
    ctx->cachedIndex = 0;
    ctx->cachedAddress = ctx->address;

}

// Set the value of an existing array entry.  This currently only works for fixed entries.
err_t arraySet(array *ctx, int index, void *data)
{

    // Append if appropriate
    if (index == ctx->count) {
        return arrayAppend(ctx, data);
    }

    // Exit if a string array
    if (ctx->size == 0) {
        return errF("not yet implemented for string arrays");
    }

    if (data == NULL) {
        memset(arrayEntry(ctx, index), 0, ctx->size);
    } else {
        memmove(arrayEntry(ctx, index), data, ctx->size);
    }

    return errNone;
}

// Shrink an array to just what's needed
void arrayShrink(array *ctx)
{
    if (ctx->allocated != ctx->length) {
        void *newData;
        err_t err = memDup(ctx->address, ctx->length, &newData);
        if (err) {
            return;
        }
        memFree(ctx->address);
        ctx->address = newData;
        ctx->allocated = ctx->length;
        ctx->cachedIndex = 0;
        ctx->cachedAddress = ctx->address;
    }
    return;
}

// Append bytes to the end of a growable object
err_t arrayAppendBytes(array *ctx, void *data, uint16_t datalen)
{
    // Note that size == 0 has a very special meaning in arrayAppend and
    // if we set size to 0 it would append 1 null byte instead (!), and so
    // we immediately return if appending nothing.
    if (datalen == 0) {
        return errNone;
    }
    ctx->size = datalen;
    return arrayAppend(ctx, data);
}

// Append a string as bytes to the end of a growable object, always leaving
// the array null-terminated even though the null byte isn't reflected
// in the array length.
err_t arrayAppendStringBytes(array *ctx, char *data)
{
    err_t err = arrayAppendBytes(ctx, data, strlen(data));
    if (!err) {
        err = arrayAppendStringTerminate(ctx);
        if (!err) {
            // Remove the null terminator from the length, so we
            // can do repeated string appends.
            ctx->length--;
            ctx->count--;
            ctx->cachedIndex = 0;
            ctx->cachedAddress = ctx->address;
        }
    }
    return err;
}

// Terminate a dynamic string AND include the null byte in the final count.
err_t arrayAppendStringTerminate(array *ctx)
{
    uint8_t databyte = 0;
    return arrayAppendBytes(ctx, &databyte, 1);
}

// Append something to a growable object, updating that object's ptr and length
err_t arrayAppend(array *ctx, void *data)
{

    err_t err;

    // If no entry size, it's a string array
    uint16_t datalen = ctx->size;
    if (datalen == 0) {
        if (data == NULL) {
            datalen = 1;
        } else {
            datalen = strlen(data)+1;
        }
    }

    // Default the chunk size
    if (ctx->chunksize == 0) {
        ctx->chunksize = 512;
    }

    // If this is the first allocation, assume that it will be small
    uint16_t chunksize = ctx->chunksize;
    if (ctx->address == NULL) {
        chunksize /= 2;
    }
    if (ctx->allocated >= (4*chunksize)) {
        chunksize = 2*chunksize;
    }

    // Determine alloc length so as to ensure that there will be room after alloc
    uint32_t alloclen = GMAX(chunksize, datalen);

    // Allocate or grow the buffer
    if (ctx->address == NULL) {
        uint8_t *initial;
        err = memAlloc(alloclen, &initial);
        if (err) {
            return err;
        }
        ctx->address = initial;
        ctx->allocated = alloclen;
        ctx->cachedIndex = 0;
        ctx->cachedAddress = ctx->address;
    } else if (datalen > (ctx->allocated - ctx->length)) {
        uint32_t cachedOffset = (uint32_t) ((uint8_t *)ctx->cachedAddress - (uint8_t *)ctx->address);
        if (ctx->cachedAddress == NULL) {
            cachedOffset = 0;
        }
        err_t err = memRealloc(ctx->allocated, ctx->allocated + alloclen, &ctx->address);
        if (err) {
            return err;
        }
        ctx->allocated = ctx->allocated + alloclen;
        if (cachedOffset == 0) {
            ctx->cachedAddress = ctx->address;
            ctx->cachedIndex = 0;
        } else {
            ctx->cachedAddress = (uint8_t *) ctx->address + cachedOffset;
        }
    }

    // Append the new stuff
    char *p = ctx->address;
    if (data == NULL) {
        memset(&p[ctx->length], 0, datalen);
    } else {
        memmove(&p[ctx->length], data, datalen);
    }
    ctx->length += datalen;
    ctx->count++;

    // Done
    return errNone;

}

// Reset an array entry
void arrayResetEntry(array *ctx, int i)
{
    if (ctx->size > 0) {
        void *p = arrayEntry(ctx, i);
        if (ctx->resetFn != NULL) {
            ctx->resetFn(p);
        }
        memset(p, 0, ctx->size);
    }
}

// Clear an array by freeing its contents but not deallocating it
void arrayClear(array *ctx)
{
    // Call the reset handler on each entry to free sub-structures
    if (ctx->resetFn != NULL && ctx->address != NULL && ctx->count != 0 && ctx->size != 0) {
        uint8_t *p = ctx->address;
        for (int i=0; i<ctx->count; i++) {
            ctx->resetFn(p);
            p += ctx->size;
        }
    }
    // Set the count to 0 indicating that it's cleared
    ctx->count = 0;
    ctx->length = 0;
    ctx->cachedIndex = 0;
    ctx->cachedAddress = ctx->address;
}

// Reset an array by freeing its contents
void arrayReset(array *ctx)
{

    // First reset the entries
    arrayClear(ctx);

    // Free the buffer containing all entries
    ctx->allocated = 0;
    if (ctx->address != NULL) {
        memFree(ctx->address);
        ctx->address = NULL;
    }

}

// Allocate an array with either a fixed size, or a string array if fixedsize==0
err_t arrayAlloc(uint16_t fixedsize, arrayEntryReset_t entryReset, array **ctx)
{
    err_t err;
    array *new;
    err = memAlloc(sizeof(array), &new);
    if (err) {
        return err;
    }
    new->size = fixedsize;
    new->resetFn = entryReset;
    *ctx = new;
    return errNone;
}

// Duplicate an array
err_t arrayDup(array *ctx, array **dupCtx)
{
    err_t err;

    array *newCtx;
    err = memDup(ctx, sizeof(array), &newCtx);
    if (err) {
        return err;
    }

    if (ctx->address != NULL) {

        void *newData;
        err = memDup(ctx->address, ctx->allocated, &newData);
        if (err) {
            memFree(newCtx);
            return err;
        }

        newCtx->address = newData;
        newCtx->cachedIndex = 0;
        newCtx->cachedAddress = newCtx->address;

    }

    *dupCtx = newCtx;
    return errNone;

}

// Free an array
void arrayFree(array *ctx)
{
    arrayReset(ctx);
    memFree(ctx);
}

// Free an array while detaching its object
void *arrayFreeDetach(array *ctx)
{
    void *p = ctx->address;
    memFree(ctx);
    return p;
}

// Free an array while optimally duplicating its object for size
void *arrayFreeDetachDup(array *ctx)
{
    void *p = ctx->address;
    if (p != NULL) {
        void *q;
        if (memDup(p, ctx->length, &q) == errNone) {
            memFree(p);
            p = q;
        }
    }
    memFree(ctx);
    return p;
}

// Sort an array
err_t arraySort(array *ctx)
{
    err_t err;
    void *l, *r, *tmp;
    if (ctx->address == NULL || ctx->count == 0) {
        return errNone;
    }
    if (ctx->isLessFn == NULL) {
        return errF("no sort method specified");
    }
    err = memAlloc(ctx->size, &tmp);
    if (err) {
        return err;
    }
    int a = 0;
    int b = ctx->count;
    for (int i = a + 1; i < b; i++) {
        for (int j = i; j > a && ctx->isLessFn(l=arrayEntry(ctx, j), r=arrayEntry(ctx, j-1)); j--) {
            memcpy(tmp, l, ctx->size);
            memcpy(l, r, ctx->size);
            memcpy(r, tmp, ctx->size);
        }
    }
    memFree(tmp);
    return errNone;
}


// Array index
void *arrayEntry(array *ctx, int index)
{

    // Handle out of range
    if (index >= ctx->count) {
        return NULL;
    }

    // If there's an entry size, use it
    if (ctx->size) {
        char *address = ctx->address;
        return (address + (ctx->size * index));
    }

    // If going backward, reset the cache
    if (index < ctx->cachedIndex) {
        ctx->cachedIndex = 0;
        ctx->cachedAddress = ctx->address;
    }

    // Use our cached index to save some iterations of this expensive loop
    char *address = ctx->cachedAddress;
    for (int i=ctx->cachedIndex; i<index; i++) {
        address += strlen(address)+1;
    }

    // Update the cached index for next iteration
    ctx->cachedIndex = index;
    ctx->cachedAddress = address;
    return address;

}

// Free entries in a map array without shrinking the allocated objects
void arrayMapClear(arrayMap *map)
{
    arrayClear(map->value1);
    arrayClear(map->value2);
    arrayClear(map->name);
    map->count = 0;
}

// Allocate a map array
err_t arrayMapAlloc(uint16_t fixedsize1, arrayEntryReset_t entryReset1, uint16_t fixedsize2, arrayEntryReset_t entryReset2, arrayMap **ctx)
{
    err_t err;

    // Allocate the map meta-structure
    arrayMap *map;
    err = memAlloc(sizeof(arrayMap), &map);
    if (err) {
        return err;
    }

    // Allocate the primary value map
    err = arrayAlloc(fixedsize1, entryReset1, &map->value1);
    if (err) {
        memFree(map);
        return err;
    }

    // Allocate the auxiliary value map
    err = arrayAlloc(fixedsize2, entryReset2, &map->value2);
    if (err) {
        arrayFree(map->value1);
        memFree(map);
        return err;
    }

    // Allocate the name map
    err = arrayAlloc(0, NULL, &map->name);
    if (err) {
        arrayFree(map->value1);
        arrayFree(map->value2);
        memFree(map);
        return err;
    }

    // Done
    *ctx = map;
    return errNone;

}

// Duplicate a map array
err_t arrayMapDup(arrayMap *in, arrayMap **out)
{
    err_t err;

    arrayMap *new;
    err = memDup(in, sizeof(arrayMap), &new);
    if (err) {
        return err;
    }

    err = arrayDup(in->name, &new->name);
    if (err) {
        memFree(new);
        return err;
    }

    err = arrayDup(in->value1, &new->value1);
    if (err) {
        memFree(new->name);
        memFree(new);
        return err;
    }

    err = arrayDup(in->value2, &new->value2);
    if (err) {
        memFree(new->value1);
        memFree(new->name);
        memFree(new);
        return err;
    }

    *out = new;
    return errNone;
}

// Shrink a map array
void arrayMapShrink(arrayMap *map)
{
    arrayShrink(map->value1);
    arrayShrink(map->value2);
    arrayShrink(map->name);
}

// Free a map array
void arrayMapFree(arrayMap *map)
{
    arrayFree(map->value1);
    arrayFree(map->value2);
    arrayFree(map->name);
    memFree(map);
}

// Free a map array, detaching its value array
void arrayMapFreeDetachValue(arrayMap *map, array **retValue1, array **retValue2)
{
    *retValue1 = map->value1;
    *retValue2 = map->value2;
    arrayFree(map->name);
    memFree(map);
}

// Remove a value in a map array
err_t arrayMapRemove(arrayMap *map, bool caseSensitive, char *name)
{

    // Remove if it's found
    uint16_t namestrlen = strlen(name)+1;
    for (int i=0; i<map->name->count; i++) {
        char *entryName = arrayEntry(map->name, i);
        bool match;
        if (caseSensitive) {
            match = memeql(name, entryName, namestrlen);
        } else {
            match = memeqlCI(name, entryName, namestrlen);
        }
        if (match) {
            arrayRemove(map->name, i);
            arrayRemove(map->value1, i);
            arrayRemove(map->value2, i);
            map->count = map->name->count;
            break;
        }
    }

    // Done
    return errNone;

}

// Set the value in a map array
err_t arrayMapSet(arrayMap *map, bool caseSensitive, char *name, void *value1, void *value2)
{
    err_t err;

    // Defensive check
    if (map == NULL || map->name == NULL || map->value1 == NULL || map->value2 == NULL) {
        debugSoftPanic("invalid map");
        return errF("invalid map");
    }

    // Remove if it's found
    uint16_t namestrlen = strlen(name)+1;
    for (int i=0; i<map->name->count; i++) {
        char *entryName = arrayEntry(map->name, i);
        bool match;
        if (caseSensitive) {
            match = memeql(name, entryName, namestrlen);
        } else {
            match = memeqlCI(name, entryName, namestrlen);
        }
        if (match) {
            arrayRemove(map->name, i);
            arrayRemove(map->value1, i);
            arrayRemove(map->value2, i);
            map->count = map->name->count;
            break;
        }
    }

    // Exit if both values are NULL, which means that we're removing it
    if (value1 == NULL && value2 == NULL) {
        return errNone;
    }

    // Append to both, knowing that arraySet works for both fixed and variable datatypes when appending
    int newEntry = map->name->count;
    err = arraySet(map->value1, newEntry, value1);
    if (err) {
        return err;
    }
    err = arraySet(map->value2, newEntry, value2);
    if (err) {
        arrayRemove(map->value1, newEntry);
        return err;
    }
    err = arraySet(map->name, newEntry, name);
    if (err) {
        arrayRemove(map->value1, newEntry);
        arrayRemove(map->value2, newEntry);
        return err;
    }
    map->count = map->name->count;

    // Done
    return errNone;

}

// Get the value of an array entry by name
bool arrayMapGet(arrayMap *map, bool caseSensitive, char *name, void *value1, void *value2)
{
    if (map == NULL || map->name == NULL || map->value1 == NULL || map->value2 == NULL) {
        debugSoftPanic("invalid map");
        return false;
    }
    uint16_t namestrlen = strlen(name)+1;
    for (int i=0; i<map->name->count; i++) {
        char *entryName = arrayEntry(map->name, i);
        bool match;
        if (caseSensitive) {
            match = memeql(name, entryName, namestrlen);
        } else {
            match = memeqlCI(name, entryName, namestrlen);
        }
        if (match) {
            if (value1 != NULL) {
                *((void **)value1) = arrayEntry(map->value1, i);
            }
            if (value2 != NULL) {
                *((void **)value2) = arrayEntry(map->value2, i);
            }
            return true;
        }
    }
    return false;
}

// Get the value of an array entry by index
bool arrayMapEntry(arrayMap *map, int index, char **name, void *value1, void *value2)
{
    if (map == NULL || map->name == NULL || map->value1 == NULL || map->value2 == NULL) {
        debugSoftPanic("invalid map");
        return false;
    }
    if (index >= map->count) {
        return false;
    }
    *name = arrayEntry(map->name, index);
    if (value1 != NULL) {
        *((void **)value1) = arrayEntry(map->value1, index);;
    }
    if (value2 != NULL) {
        *((void **)value2) = arrayEntry(map->value2, index);;
    }
    return true;
}

// For a string array, join all entries into a single entry using
// the specified character as a separator.  It's guaranteed that
// after this is called there is exactly 1 entry in the array.
void arrayJoinString(array *ctx, char sep)
{
    // Should never happen, but defensive programming
    if (ctx->size != 0) {
        ctx->size = ctx->length = ctx->count = 0;
    }
    // If nothing in the array, append a single blank string
    if (ctx->count == 0) {
        arrayAppend(ctx, "");
    }
    // Defensive
    if (ctx->count <= 1 || ctx->length == 0) {
        return;
    }
    // Do the join
    char *addr = (char *) ctx->address;
    for (int i=0; i<ctx->length-1; i++) {
        if (addr[i] == '\0') {
            addr[i] = sep;
        }
    }
    // There's now but a single entry
    ctx->count = 1;
}

// For a string array, split a single entry into multiple entries using
// the specified character as a separator.
err_t arraySplitString(array *ctx, char sep)
{

    // Should never happen, but defensive programming
    if (ctx->size != 0) {
        ctx->size = ctx->length = ctx->count = 0;
    }

    // Defensive
    if (ctx->count != 1 || ctx->length == 0) {
        return errF("array cannot be split");
    }

    // Do the split
    char *firstEntry = arrayEntry(ctx,0);
    if (strchr(firstEntry, sep)) {
        uint32_t alloclen = strlen(firstEntry)+1;
        err_t err = memAlloc(alloclen, &firstEntry);
        if (err) {
            return err;
        }

        strlcpy(firstEntry, arrayEntry(ctx,0), alloclen);

        // Remove the original entry;
        arrayRemove(ctx, 0);

        // iterate over string finding entries and separators.  Do not use strtok
        // because it discards empty entries.
        char *pentry, *psep;
        pentry = firstEntry;

        while(pentry != NULL) {
            psep = strchr(pentry, sep);
            if (psep != NULL) {
                *psep++ = '\0';
            }
            arrayAppend(ctx, pentry);
            pentry = psep;
        }

        memFree(firstEntry);
    }

    return errNone;
}
