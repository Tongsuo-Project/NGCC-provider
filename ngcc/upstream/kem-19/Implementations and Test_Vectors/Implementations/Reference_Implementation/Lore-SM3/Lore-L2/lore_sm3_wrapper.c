#include "lore_sm3_wrapper.h"
#include "auxfunc.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

static void clear_bytes(void *ptr, size_t length)
{
    volatile uint8_t *p = (volatile uint8_t *)ptr;
    while (length--) *p++ = 0;
}

/* Build LORE || version || backend || level || domain || BE64(len) ||
 * BE32(context counter) || input. The counter is separate from KDF-SM3's
 * internal block counter, so sampling nonces and matrix coordinates cannot
 * share a stream accidentally. Input is never prehashed to 256 bits here. */
static uint8_t *frame(uint8_t domain, uint32_t counter,
                      const uint8_t *in, size_t inlen, size_t extra)
{
    /* Leave room for the official routines' padding, HMAC prefixes and
     * internal +255-bit rounding, not only our own allocation. */
    if (inlen > SIZE_MAX - 128 - LORE_FRAME_BYTES - extra ||
        inlen > (ULLONG_MAX - 1024ULL) / 8 - LORE_FRAME_BYTES - extra ||
        (inlen != 0 && in == NULL)) return NULL;
    uint8_t *buf = malloc(LORE_FRAME_BYTES + inlen + extra);
    if (buf == NULL) return NULL;
    lore_frame_header(buf, domain, counter, inlen);
    if (inlen != 0) memcpy(buf + LORE_FRAME_BYTES, in, inlen);
    return buf;
}

int lore_sm3_hash(uint8_t *out, size_t outlen, uint8_t domain, uint32_t counter,
                  const uint8_t *in, size_t inlen)
{
    int bits;
    if (out == NULL) return -1;
    switch (outlen) {
    case 32: bits = 256; break;
    case 48: case 64: bits = 512; break;
    case 96: bits = 768; break;
    case 128: bits = 1024; break;
    default: return -1;
    }
    uint8_t *buf = frame(domain, counter, in, inlen, 0);
    if (buf == NULL) return -1;
    uint8_t digest[128];
    const unsigned long long input_bits =
        (unsigned long long)(LORE_FRAME_BYTES + inlen) * 8ULL;
    int ret = bits == 256 ? sm3hash(bits, buf, input_bits, digest) :
                           pseudohash(bits, buf, input_bits, digest);
    if (ret == 0) memcpy(out, digest, outlen); /* 48 bytes: prefix of hash512. */
    clear_bytes(digest, sizeof digest);
    clear_bytes(buf, LORE_FRAME_BYTES + inlen);
    free(buf);
    return ret;
}

int lore_sm3_xof(uint8_t *out, size_t outlen, uint8_t domain, uint32_t counter,
                 const uint8_t *in, size_t inlen)
{
    /* API_PKC specifies output_bits < 2^40 - 2^8. */
    const unsigned long long max_bytes = (1ULL << 37) - 32ULL;
    if (outlen > SIZE_MAX - 32 || (unsigned long long)outlen >= max_bytes ||
        (outlen != 0 && out == NULL)) return -1;
    if (outlen == 0) return 0;
    uint8_t *buf = frame(domain, counter, in, inlen, 0);
    if (buf == NULL) return -1;
    int ret = pseudoXOF((unsigned long long)outlen * 8ULL, buf,
        (unsigned long long)(LORE_FRAME_BYTES + inlen) * 8ULL, out);
    clear_bytes(buf, LORE_FRAME_BYTES + inlen);
    free(buf);
    return ret;
}

int lore_sm3_xof_read(uint8_t *out, size_t outlen, size_t offset,
                      uint8_t domain, uint32_t counter,
                      const uint8_t *in, size_t inlen)
{
    const unsigned long long max_bytes = (1ULL << 37) - 32ULL;
    if (offset > SIZE_MAX - outlen ||
        (unsigned long long)offset + (unsigned long long)outlen >= max_bytes ||
        (outlen != 0 && out == NULL)) return -1;
    if (outlen == 0) return 0;
    uint8_t *buf = frame(domain, counter, in, inlen, 4);
    if (buf == NULL) return -1;
    const size_t length = LORE_FRAME_BYTES + inlen;
    uint8_t digest[32];
    size_t written = 0;
    int ret = 0;
    /* Exactly pseudoXOF's SM3(frame || BE32(block)), using the official
     * sm3hash API to read only the required blocks, not the whole prefix. */
    while (written < outlen) {
        const size_t position = offset + written;
        const uint32_t block = (uint32_t)(position / 32) + 1;
        for (unsigned int i = 0; i < 4; ++i)
            buf[length + i] = (uint8_t)(block >> (24 - 8 * i));
        ret = sm3hash(256, buf, (unsigned long long)(length + 4) * 8ULL, digest);
        if (ret != 0) break;
        const size_t start = position % 32;
        size_t take = 32 - start;
        if (take > outlen - written) take = outlen - written;
        memcpy(out + written, digest + start, take);
        written += take;
    }
    clear_bytes(digest, sizeof digest);
    clear_bytes(buf, length + 4);
    free(buf);
    return ret;
}

void lore_hash_h(uint8_t out[LORE_SYMBYTES], uint8_t domain,
                 const uint8_t *in, size_t inlen)
{
    if (lore_sm3_hash(out, LORE_SYMBYTES, domain, 0, in, inlen) != 0) abort();
}

void lore_hash_g(uint8_t out[2 * LORE_SYMBYTES],
                 const uint8_t *in, size_t inlen)
{
    if (lore_sm3_hash(out, 2 * LORE_SYMBYTES, LORE_DOMAIN_G, 0, in, inlen) != 0)
        abort();
}

void lore_kdf(uint8_t out[LORE_SYMBYTES], const uint8_t *in, size_t inlen)
{
    if (lore_sm3_xof(out, LORE_SYMBYTES, LORE_DOMAIN_KDF, 0, in, inlen) != 0)
        abort();
}

void lore_expand_seed(uint8_t out[2 * LORE_SYMBYTES],
                      const uint8_t seed[LORE_SYMBYTES])
{
    if (lore_sm3_xof(out, 2 * LORE_SYMBYTES, LORE_DOMAIN_KEYGEN, 0,
                     seed, LORE_SYMBYTES) != 0) abort();
}
