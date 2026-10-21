#include <string.h>
#include <stdlib.h>

#include "gf2x.h"

#define K_SQR_THRESHOLD 0

#define INV_MAX_ITER 16

const size_t k0[] = {0, 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384};
const size_t t0[] = {0, 16494, 8247, 26802, 22292, 17096, 8396, 32584, 30461, 14185, 26512, 32135, 190, 3113, 25578, 2913};
const size_t k1[] = {0, 0, 0, 1, 9, 0, 25, 89, 0, 0, 0, 0, 0, 0, 0, 217};
const size_t t1[] = {0, 0, 0, 16494, 11146, 0, 19104, 20046, 0, 0, 0, 0, 0, 0, 0, 31836};


// Multiplies two polynomials.
static inline void schoolbook_mul(uint64_t *c, const uint64_t *a, const uint64_t *b, size_t n64)
{
    for (size_t i = 0; i < n64; i++)
    {
        uint64_t mask = - (a[i] & 1);
        for (size_t j = 0; j < n64; j++)
        {
            c[i + j] ^= b[j] & mask;
        }
        for (size_t bit = 1; bit < 64; bit++)
        {
            mask = - ((a[i] >> bit) & 1);
            size_t low_bits = 64 - bit;
            for (size_t j = 0; j < n64; j++)
            {
                c[i + j] ^= (b[j] << bit) & mask;
                c[i + j + 1] ^= (b[j] >> low_bits) & mask;
            }
        }
    }
}

// Adds two vectors.
static inline void gf2x_add_len(uint64_t *c, const uint64_t *a, const uint64_t *b, size_t n64)
{
    for (size_t i = 0; i < n64; i++)
    {
        c[i] = a[i] ^ b[i];
    }
}

// Adds three vectors.
static inline void gf2x_add3_len(uint64_t *c, const uint64_t *a, const uint64_t *b, const uint64_t *d, size_t n64)
{
    for (size_t i = 0; i < n64; i++)
    {
        c[i] = a[i] ^ b[i] ^ d[i];
    }
}

// Multiplies two polynomials.
void karatsuba_mul(uint64_t *c, const uint64_t *a, const uint64_t *b, size_t n64, uint64_t *buf)
{
    if (n64 <= 1)
    {
        memset(c, 0, n64 * 2 * 8);
        schoolbook_mul(c, a, b, n64);
        return;
    }

    size_t m64 = n64 / 2;
    const uint64_t *a_low = a, *a_high = a + m64;
    const uint64_t *b_low = b, *b_high = b + m64;
    uint64_t *a_sum = buf, *b_sum = buf + m64;
    uint64_t *z0 = buf + n64, *z1 = buf + n64 * 2, *z2 = buf + n64 * 3;
    uint64_t *buf_next = buf + n64 * 4;

    gf2x_add_len(a_sum, a_low, a_high, m64);
    gf2x_add_len(b_sum, b_low, b_high, m64);

    karatsuba_mul(z0, a_low, b_low, m64, buf_next);
    karatsuba_mul(z2, a_high, b_high, m64, buf_next);
    karatsuba_mul(z1, a_sum, b_sum, m64, buf_next);

    gf2x_add3_len(z1, z1, z0, z2, n64);

    fast_cpy(c, z0, n64);
    fast_cpy(c + n64, z2, n64);
    gf2x_add_len(c + m64, z1, c + m64, n64);
}

// Reduces a polynomial modulo x^r - 1.
static inline void gf2x_mod(uint64_t *c, const uint64_t *a)
{
    memcpy(c, a, R_SIZE_64 << 3);
    for (size_t i = 0; i < R_SIZE_64; i++)
    {
        c[i] ^= (a[i + R_SIZE_64 - 1] >> R_64_LAST_BITS) | (a[i + R_SIZE_64] << R_64_REST_BITS);
    }
    c[R_SIZE_64 - 1] &= R_64_LAST_MASK;
}

// Adds two polynomials.
void gf2x_add(uint8_t *c, const uint8_t *a, const uint8_t *b)
{
    for (size_t i = 0; i < R_SIZE_BYTES; i++)
    {
        c[i] = a[i] ^ b[i];
    }
}

// Multiplies and reduces two polynomials.
int gf2x_mul(uint8_t *c, const uint8_t *a, const uint8_t *b)
{
    uint64_t *temp = (uint64_t *)trike_alloc(PADDED_R_SIZE_64 * 2 * sizeof(uint64_t));
    uint64_t *buf = (uint64_t *)trike_alloc(PADDED_R_SIZE_64 * 8 * sizeof(uint64_t));

    if (!temp || !buf) { free(temp); free(buf); return -1; }
    // Karatsuba initializes every output and scratch word before reading it.
    // In particular, its leaves overwrite 2*n64 words (or clear before XOR
    // in the portable implementation), so these buffers need no preclear.

    uint64_t *words = (uint64_t *)calloc(PADDED_R_SIZE_64 * 2, sizeof(uint64_t));
    if (!words) { free(temp); free(buf); return -1; }
    uint64_t *a_words = words, *b_words = words + PADDED_R_SIZE_64;

    // Polynomial bytes are little-endian, independent of host byte order.
    for (size_t i = 0; i < R_SIZE_BYTES; i++)
    {
        a_words[i / 8] |= (uint64_t)a[i] << ((i % 8) * 8);
        b_words[i / 8] |= (uint64_t)b[i] << ((i % 8) * 8);
    }
    karatsuba_mul(temp, a_words, b_words, PADDED_R_SIZE_64, buf);
    gf2x_mod(a_words, temp);
    for (size_t i = 0; i < R_SIZE_BYTES; i++)
    {
        c[i] = (uint8_t)(a_words[i / 8] >> ((i % 8) * 8));
    }
    memset(c + R_SIZE_BYTES, 0, R_ZMM_SIZE_BYTES - R_SIZE_BYTES);
    free(words);

    free(buf);
    free(temp);
    return 0;
}

// Squares and reduces a polynomial.
static inline int gf2x_sqr(uint8_t *c, const uint8_t *a)
{
    return gf2x_mul(c, a, a);
}

// Computes repeated squaring.
static inline int gf2x_k_sqr(uint8_t *c, const uint8_t *a, size_t k, size_t t)
{
    if (k <= K_SQR_THRESHOLD)
    {
        memcpy(c, a, R_ZMM_SIZE_BYTES);
        for (size_t i = 0; i < k; i++)
        {
            if (gf2x_sqr(c, c)) return -1;
        }
    }
    else
    {
        uint64_t idx = 0;
        size_t pos = 0;
        for (size_t i = 0; i < R_SIZE_BYTES; i++)
        {
            uint8_t byte = 0;
            for (size_t bit = 0; bit < 8 && idx < PARAM_R; bit++, idx++)
            {
                byte |= (a[pos >> 3] >> (pos & 7) & 1) << bit;
                pos += t;
                pos -= -(pos >= PARAM_R) & PARAM_R;
            }
            c[i] = byte;
        }
    }
    return 0;
}

// Inverts a polynomial.
int gf2x_inv(uint8_t *inv_a, const uint8_t *a)
{
    uint8_t *f = (uint8_t *)trike_alloc(PADDED_R_SIZE_BYTES);
    uint8_t *g = (uint8_t *)trike_alloc(PADDED_R_SIZE_BYTES);

    int rc = -1;
    if (!f || !g) goto cleanup;
    trike_assign(a, f, R_ZMM_SIZE_BYTES, PADDED_R_SIZE_BYTES);
    trike_assign(a, inv_a, R_ZMM_SIZE_BYTES, PADDED_R_SIZE_BYTES);
    trike_setz(g, PADDED_R_SIZE_BYTES);

    for (size_t i = 1; i < INV_MAX_ITER; i++)
    {
        if (gf2x_k_sqr(g, f, k0[i], t0[i])) goto cleanup;
        if (gf2x_mul(f, f, g)) goto cleanup;
        if(k1[i])
        {
            if (gf2x_k_sqr(g, f, k1[i], t1[i])) goto cleanup;
            if (gf2x_mul(inv_a, inv_a, g)) goto cleanup;
        }
    }
    rc = gf2x_sqr(inv_a, inv_a);
cleanup:

    free(f);
    free(g);
    return rc;
}


// Applies a cyclic shift.
int gf2x_shift(uint8_t *out, const uint8_t *in, uint32_t shift)
{

    uint8_t *temp = (uint8_t *)calloc(R_SIZE_BYTES * 2, sizeof(uint8_t));
    if (temp == NULL) return -1;

    memset(temp, 0, R_SIZE_BYTES * 2);

    uint32_t byte_shift = shift / 8, bit_shift = shift % 8, low_bits = 8 - bit_shift;
    temp[byte_shift] = in[0] << bit_shift;
    for (uint32_t i = 0; i < R_SIZE_BYTES - 1; i++)
    {
        temp[byte_shift + i + 1] = (in[i] >> low_bits) | (in[i + 1] << bit_shift);
    }
    temp[byte_shift + R_SIZE_BYTES] = (in[R_SIZE_BYTES - 1] >> low_bits);

    memcpy(out, temp, R_SIZE_BYTES);

    uint32_t high_bits = (PARAM_R - 1) % 8 + 1; low_bits = 8 - high_bits;

    for (uint32_t i = 0; i < R_SIZE_BYTES; i++)
    {
        out[i] ^= (temp[i + R_SIZE_BYTES - 1] >> high_bits) | (temp[i + R_SIZE_BYTES] << low_bits);
    }

    out[R_SIZE_BYTES - 1] &= (1U << high_bits) - 1;

    free(temp);

    return 0;
}
