/* See amd5.c for explanation and copyright information.  */

#ifndef AMD5_H
#define AMD5_H

/* Unlike previous versions of this code, uint32 need not be exactly
   32 bits, merely 32 bits or more.  Choosing a data type which is 32
   bits instead of 64 is not important; speed is considerably more
   important.  ANSI guarantees that "unsigned long" will be big enough,
   and always using it seems to have few disadvantages.  */
typedef unsigned char uint8;
typedef unsigned long uint32;
typedef unsigned long long uint64;

#define MD5_DIGEST_SIZE		16
#define MD5_HMAC_BLOCK_SIZE	64
#define MD5_BLOCK_WORDS		16
#define MD5_HASH_WORDS		4

struct MD5Context {
    uint32 buf[4];
    uint32 bits[2];
    uint8 in[64];
    uint32 hash[MD5_HASH_WORDS];
    uint32 block[MD5_BLOCK_WORDS];
    uint64 byte_count;
};

void MD5Init(struct MD5Context *ctx);
void MD5Update(struct MD5Context *ctx, unsigned char const *buf, uint32 len);
void MD5Final(unsigned char digest[16], struct MD5Context *ctx);

/*
 * This is needed to make RSAREF happy on some MS-DOS compilers.
 */
typedef struct MD5Context MD5_CTX;

#endif /* !MD5_H */
