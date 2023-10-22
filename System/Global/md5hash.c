// Copyright 2017 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "global.h"

// Hash data and return its binary hash into an MD5_SIZE buffer
void md5Hash(uint8_t* data, uint32_t len, uint8_t *retHash)
{

    struct MD5Context context;
    MD5Init(&context);
    MD5Update(&context, data, len);
    MD5Final(retHash, &context);
}

// Hash data and return it as a string into a buffer that is at least (16*2)+1 in length
void md5HashToString(uint8_t *data, uint32_t len, char *strbuf, uint32_t buflen)
{
    uint8_t hash[MD5_SIZE];
    md5Hash(data, len, hash);
    md5BinaryToString(hash, strbuf, buflen);
}

// Convert MD5 binary to string into a buffer that is at least MD5_HEX_STRING_SIZE in length
void md5BinaryToString(uint8_t *hash, char *strbuf, uint32_t buflen)
{
    char hashstr[MD5_SIZE*3] = {0};
    for (int i=0; i<MD5_SIZE; i++) {
        snprintf((char *)&hashstr[i*2], 5, "%02x", hash[i]);
    }
    strlcpy(strbuf, hashstr, buflen);
}
