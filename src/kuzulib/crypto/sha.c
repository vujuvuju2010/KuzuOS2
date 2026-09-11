/* KuzuOS C Library - SHA-1 and SHA-256 implementation for Tor compatibility */
#include <stdint.h>
#include <string.h>

/* SHA-1 Context */
typedef struct {
    uint32_t state[5];
    uint32_t count[2];
    uint8_t  buffer[64];
} sha1_ctx_t;

/* SHA-256 Context */
typedef struct {
    uint32_t state[8];
    uint32_t count[2];
    uint8_t  buffer[64];
} sha256_ctx_t;

/* SHA-1 constants and macros */
#define SHA1_K0  0x5A827999
#define SHA1_K20 0x6ED9EBA1
#define SHA1_K40 0x8F1BBCDC
#define SHA1_K60 0xCA62C1D6

#define SHA1_H0  0x67452301
#define SHA1_H1  0xEFCDAB89
#define SHA1_H2  0x98BADCFE
#define SHA1_H3  0x10325476
#define SHA1_H4  0xC3D2E1F0

#define SHA1_F1(x,y,z) (z ^ (x & (y ^ z)))
#define SHA1_F2(x,y,z) (x ^ y ^ z)
#define SHA1_F3(x,y,z) ((x & y) | (z & (x | y)))
#define SHA1_F4(x,y,z) (x ^ y ^ z)
#define SHA1_ROT(x,n) (((x) << n) | ((x) >> (32-n)))

/* SHA-256 constants */
static const uint32_t sha256_k[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

#define SHA256_H0 0x6a09e667
#define SHA256_H1 0xbb67ae85
#define SHA256_H2 0x3c6ef372
#define SHA256_H3 0xa54ff53a
#define SHA256_H4 0x510e527f
#define SHA256_H5 0x9b05688c
#define SHA256_H6 0x1f83d9ab
#define SHA256_H7 0x5be0cd19

#define SHA256_ROTR(x,n) (((x) >> n) | ((x) << (32-n)))
#define SHA256_CH(x,y,z)  (z ^ (x & (y ^ z)))
#define SHA256_MAJ(x,y,z) ((x & y) | (z & (x | y)))
#define SHA256_SIG0(x) (SHA256_ROTR(x,2) ^ SHA256_ROTR(x,13) ^ SHA256_ROTR(x,22))
#define SHA256_SIG1(x) (SHA256_ROTR(x,6) ^ SHA256_ROTR(x,11) ^ SHA256_ROTR(x,25))
#define SHA256_GAMMA0(x) (SHA256_ROTR(x,7) ^ SHA256_ROTR(x,18) ^ (x >> 3))
#define SHA256_GAMMA1(x) (SHA256_ROTR(x,17) ^ SHA256_ROTR(x,19) ^ (x >> 10))

/* Byte swapping for big-endian */
static inline uint32_t be32(uint32_t x) {
    return ((x & 0xFF000000) >> 24) |
           ((x & 0x00FF0000) >>  8) |
           ((x & 0x0000FF00) <<  8) |
           ((x & 0x000000FF) << 24);
}

/* SHA-1 Transform */
static void sha1_transform(sha1_ctx_t* ctx, const uint8_t data[64]) {
    uint32_t a, b, c, d, e, t;
    uint32_t w[80];
    int i;
    
    /* Prepare message schedule */
    for (i = 0; i < 16; i++) {
        w[i] = be32(*(uint32_t*)(data + i*4));
    }
    for (i = 16; i < 80; i++) {
        w[i] = SHA1_ROT(w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16], 1);
    }
    
    /* Initialize hash values */
    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];
    e = ctx->state[4];
    
    /* Main loop */
    for (i = 0; i < 80; i++) {
        uint32_t f, k;
        
        if (i < 20) {
            f = SHA1_F1(b, c, d);
            k = SHA1_K0;
        } else if (i < 40) {
            f = SHA1_F2(b, c, d);
            k = SHA1_K20;
        } else if (i < 60) {
            f = SHA1_F3(b, c, d);
            k = SHA1_K40;
        } else {
            f = SHA1_F4(b, c, d);
            k = SHA1_K60;
        }
        
        t = SHA1_ROT(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = SHA1_ROT(b, 30);
        b = a;
        a = t;
    }
    
    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
}

/* SHA-1 Initialize */
void sha1_init(sha1_ctx_t* ctx) {
    ctx->state[0] = SHA1_H0;
    ctx->state[1] = SHA1_H1;
    ctx->state[2] = SHA1_H2;
    ctx->state[3] = SHA1_H3;
    ctx->state[4] = SHA1_H4;
    ctx->count[0] = ctx->count[1] = 0;
}

/* SHA-1 Update */
void sha1_update(sha1_ctx_t* ctx, const uint8_t* data, size_t len) {
    size_t i;
    size_t index = (ctx->count[0] >> 3) & 0x3F;
    
    ctx->count[0] += (uint32_t)(len << 3);
    if (ctx->count[0] < (uint32_t)(len << 3))
        ctx->count[1]++;
    ctx->count[1] += (uint32_t)(len >> 29);
    
    size_t partlen = 64 - index;
    
    if (len >= partlen) {
        memcpy(&ctx->buffer[index], data, partlen);
        sha1_transform(ctx, ctx->buffer);
        
        for (i = partlen; i + 63 < len; i += 64) {
            sha1_transform(ctx, &data[i]);
        }
        
        index = 0;
    } else {
        i = 0;
    }
    
    memcpy(&ctx->buffer[index], &data[i], len - i);
}

/* SHA-1 Final */
void sha1_final(uint8_t digest[20], sha1_ctx_t* ctx) {
    uint8_t padding[64] = {
        0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    
    uint8_t bits[8];
    size_t index = (ctx->count[0] >> 3) & 0x3f;
    size_t padlen = (index < 56) ? (56 - index) : (120 - index);
    
    /* Store bit count */
    for (int i = 0; i < 8; i++) {
        bits[i] = (uint8_t)(ctx->count[(i < 4) ? 0 : 1] >> 
                           ((3 - (i & 3)) * 8) & 0xff);
    }
    
    sha1_update(ctx, padding, padlen);
    sha1_update(ctx, bits, 8);
    
    /* Store digest in big-endian */
    for (int i = 0; i < 5; i++) {
        digest[i*4]     = (ctx->state[i] >> 24) & 0xFF;
        digest[i*4 + 1] = (ctx->state[i] >> 16) & 0xFF;
        digest[i*4 + 2] = (ctx->state[i] >> 8)  & 0xFF;
        digest[i*4 + 3] = (ctx->state[i])       & 0xFF;
    }
}

/* SHA-1 one-shot function */
void sha1(const uint8_t* data, size_t len, uint8_t digest[20]) {
    sha1_ctx_t ctx;
    sha1_init(&ctx);
    sha1_update(&ctx, data, len);
    sha1_final(digest, &ctx);
}

/* SHA-256 Transform */
static void sha256_transform(sha256_ctx_t* ctx, const uint8_t data[64]) {
    uint32_t a, b, c, d, e, f, g, h;
    uint32_t t1, t2;
    uint32_t w[64];
    int i;
    
    /* Prepare message schedule */
    for (i = 0; i < 16; i++) {
        w[i] = be32(*(uint32_t*)(data + i*4));
    }
    for (i = 16; i < 64; i++) {
        w[i] = SHA256_GAMMA1(w[i-2]) + w[i-7] + 
               SHA256_GAMMA0(w[i-15]) + w[i-16];
    }
    
    /* Initialize working variables */
    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];
    e = ctx->state[4];
    f = ctx->state[5];
    g = ctx->state[6];
    h = ctx->state[7];
    
    /* Main loop */
    for (i = 0; i < 64; i++) {
        t1 = h + SHA256_SIG1(e) + SHA256_CH(e, f, g) + sha256_k[i] + w[i];
        t2 = SHA256_SIG0(a) + SHA256_MAJ(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }
    
    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
    ctx->state[5] += f;
    ctx->state[6] += g;
    ctx->state[7] += h;
}

/* SHA-256 Initialize */
void sha256_init(sha256_ctx_t* ctx) {
    ctx->state[0] = SHA256_H0;
    ctx->state[1] = SHA256_H1;
    ctx->state[2] = SHA256_H2;
    ctx->state[3] = SHA256_H3;
    ctx->state[4] = SHA256_H4;
    ctx->state[5] = SHA256_H5;
    ctx->state[6] = SHA256_H6;
    ctx->state[7] = SHA256_H7;
    ctx->count[0] = ctx->count[1] = 0;
}

/* SHA-256 Update */
void sha256_update(sha256_ctx_t* ctx, const uint8_t* data, size_t len) {
    size_t i;
    size_t index = (ctx->count[0] >> 3) & 0x3F;
    
    ctx->count[0] += (uint32_t)(len << 3);
    if (ctx->count[0] < (uint32_t)(len << 3))
        ctx->count[1]++;
    ctx->count[1] += (uint32_t)(len >> 29);
    
    size_t partlen = 64 - index;
    
    if (len >= partlen) {
        memcpy(&ctx->buffer[index], data, partlen);
        sha256_transform(ctx, ctx->buffer);
        
        for (i = partlen; i + 63 < len; i += 64) {
            sha256_transform(ctx, &data[i]);
        }
        
        index = 0;
    } else {
        i = 0;
    }
    
    memcpy(&ctx->buffer[index], &data[i], len - i);
}

/* SHA-256 Final */
void sha256_final(uint8_t digest[32], sha256_ctx_t* ctx) {
    uint8_t padding[64] = {
        0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    
    uint8_t bits[8];
    size_t index = (ctx->count[0] >> 3) & 0x3f;
    size_t padlen = (index < 56) ? (56 - index) : (120 - index);
    
    for (int i = 0; i < 8; i++) {
        bits[i] = (uint8_t)(ctx->count[(i < 4) ? 0 : 1] >> 
                           ((3 - (i & 3)) * 8) & 0xff);
    }
    
    sha256_update(ctx, padding, padlen);
    sha256_update(ctx, bits, 8);
    
    for (int i = 0; i < 8; i++) {
        digest[i*4]     = (ctx->state[i] >> 24) & 0xFF;
        digest[i*4 + 1] = (ctx->state[i] >> 16) & 0xFF;
        digest[i*4 + 2] = (ctx->state[i] >> 8)  & 0xFF;
        digest[i*4 + 3] = (ctx->state[i])       & 0xFF;
    }
}

/* SHA-256 one-shot function */
void sha256(const uint8_t* data, size_t len, uint8_t digest[32]) {
    sha256_ctx_t ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, data, len);
    sha256_final(digest, &ctx);
}
