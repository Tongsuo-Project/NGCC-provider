/* Copyright (c) 2026 Hang Zhang. */

#include "rans_decode.h"

#define RANS_SCALE_BITS 16
#define RANS_SCALE (1U << RANS_SCALE_BITS)
#define RANS_BYTE_L (1U << 23)
#define RANS_TMP_BYTES (2 * (BIT_L + 1) * BIT_N + 256)
#define RANS_Z0_ZERO_FLAG 0x8000U
#define RANS_LENGTH_MASK 0x7FFFU
/* Symbol origin of the full-support frequency table. */
#define Z0_HIGH_BOUND ((int)(sizeof(z_high_freq) / sizeof(z_high_freq[0]) / 2))
#define Z0_HIGH_SYMBOLS (2 * Z0_HIGH_BOUND + 1)
#define Z0_HIGH_ZERO Z0_HIGH_BOUND
#define z0_high_freq z_high_freq
#define z0_high_enc z_high_enc

typedef struct {
    uint32_t x_max;
    uint32_t rcp_freq;
    uint32_t bias;
    uint16_t cmpl_freq;
    uint8_t rcp_shift;
} rans_enc_symbol;

#include "rans_model_256.h"

static inline int rans_enc_put(uint32_t *state, unsigned char **ptr, unsigned char *begin,
                               unsigned int symbol, const rans_enc_symbol *model,
                               unsigned int nsymbols)
{
    const rans_enc_symbol *sym;
    uint32_t x;
    uint32_t q;

    if (symbol >= nsymbols)
        return -1;
    sym = &model[symbol];
    x = *state;
    while (x >= sym->x_max) {
        if (*ptr == begin)
            return -1;
        *--*ptr = (uint8_t)x;
        x >>= 8;
    }
    q = (uint32_t)(((uint64_t)x * sym->rcp_freq) >> 32) >> sym->rcp_shift;
    *state = x + sym->bias + q * sym->cmpl_freq;
    return 0;
}

static int rans_encode_pair(unsigned char *z_out, size_t *z_out_len, const uint8_t *z_symbols,
                            size_t z_count, size_t z0_count, unsigned char *h_out,
                            size_t *h_out_len, const uint8_t *h_symbols,
                            const rans_enc_symbol *h_model, unsigned int h_nsymbols, size_t h_count)
{
    unsigned char z_tmp[RANS_TMP_BYTES], h_tmp[RANS_TMP_BYTES];
    unsigned char *z_ptr = z_tmp + sizeof(z_tmp);
    unsigned char *h_ptr = h_tmp + sizeof(h_tmp);
    uint32_t z_state = RANS_BYTE_L;
    uint32_t h_state = RANS_BYTE_L;

    /* Interleave the two independent rANS streams. */
    while (z_count != 0 || h_count != 0) {
        if (z_count != 0) {
            size_t i = --z_count;
            const rans_enc_symbol *model = i < z0_count ? z0_high_enc : z_high_enc;

            if (rans_enc_put(&z_state, &z_ptr, z_tmp, z_symbols[i], model, Z0_HIGH_SYMBOLS) != 0)
                return -1;
        }
        if (h_count != 0) {
            size_t i = --h_count;
            if (rans_enc_put(&h_state, &h_ptr, h_tmp, h_symbols[i], h_model, h_nsymbols) != 0)
                return -1;
        }
    }
    if ((size_t)(z_ptr - z_tmp) < 4 || (size_t)(h_ptr - h_tmp) < 4)
        return -1;
    z_ptr -= 4;
    h_ptr -= 4;
    store64_le_n(z_ptr, z_state, 4);
    store64_le_n(h_ptr, h_state, 4);
    *z_out_len = (size_t)(z_tmp + sizeof(z_tmp) - z_ptr);
    *h_out_len = (size_t)(h_tmp + sizeof(h_tmp) - h_ptr);
    memcpy(z_out, z_ptr, *z_out_len);
    memcpy(h_out, h_ptr, *h_out_len);
    return 0;
}

static int rans_decode_symbol(uint8_t *symbol, uint32_t *state, const uint16_t *freq,
                              const uint64_t *lookup,
                              const unsigned char **ptr, const unsigned char *end)
{
    uint32_t slot = *state & (RANS_SCALE - 1);
    uint32_t start = 0;
    uint32_t frequency;
    unsigned int value;

    value = rans_find_symbol_fast(slot, freq, lookup, &start, &frequency);
    *symbol = (uint8_t)value;
    *state = frequency * (*state >> RANS_SCALE_BITS) + slot - start;
    while (*state < RANS_BYTE_L && *ptr < end)
        *state = (*state << 8) | *(*ptr)++;
    return 0;
}

static int rans_decode_pair(uint8_t *z_symbols, size_t z_count, size_t z0_count, uint8_t *h_symbols,
                            size_t h_count, const unsigned char *z_in, size_t z_len,
                            const unsigned char *h_in, size_t h_len)
{
    const unsigned char *z_ptr = z_in;
    const unsigned char *h_ptr = h_in;
    const unsigned char *z_end = z_in + z_len;
    const unsigned char *h_end = h_in + h_len;
    uint32_t z_state, h_state;

    if (z_len < 4 || h_len < 4)
        return -1;
    z_state = (uint32_t)load64_le_n(z_ptr, 4);
    h_state = (uint32_t)load64_le_n(h_ptr, 4);
    z_ptr += 4;
    h_ptr += 4;
    if (z_state < RANS_BYTE_L || z_state >= (RANS_BYTE_L << 8) || h_state < RANS_BYTE_L ||
        h_state >= (RANS_BYTE_L << 8))
        return -1;

    for (size_t i = 0; i < z_count || i < h_count; i++) {
        if (i < z_count) {
            const uint16_t *freq = i < z0_count ? z0_high_freq : z_high_freq;
            if (rans_decode_symbol(&z_symbols[i], &z_state, freq, z_high_fast,
                                   &z_ptr, z_end) != 0)
                return -1;
        }
        if (i < h_count && rans_decode_symbol(&h_symbols[i], &h_state, h_freq, h_fast,
                                              &h_ptr, h_end) != 0)
            return -1;
    }
    return z_state == RANS_BYTE_L && h_state == RANS_BYTE_L && z_ptr == z_end && h_ptr == h_end
               ? 0
               : -1;
}

static int rans_decode_z1_h(uint8_t z_symbols[BIT_L * BIT_N], uint8_t h_symbols[BIT_K * BIT_N],
                            const unsigned char *z_in, size_t z_len, const unsigned char *h_in,
                            size_t h_len)
{
    return rans_decode_pair(z_symbols, BIT_L * BIT_N, 0, h_symbols, BIT_K * BIT_N, z_in, z_len,
                            h_in, h_len);
}

/* Every response block uses z = 256*high + low, low in [-128,127]. */
static int split_z1_symbols(uint8_t low[BIT_N], uint8_t high[BIT_N], const int16_t coeffs[BIT_N])
{
    uint32_t bad = 0;
    for (int j = 0; j < BIT_N; j++) {
        int32_t z = coeffs[j];
        int32_t hi = (z + 128) >> 8;
        bad |= (uint32_t)(z < -BIT_RESPONSE_COEFF_BOUND || z > BIT_RESPONSE_COEFF_BOUND);
        low[j] = (uint8_t)z;
        high[j] = (uint8_t)(hi + Z0_HIGH_ZERO);
    }
    return bad != 0;
}
#define split_z0_symbols split_z1_symbols

static int hint_symbols(uint8_t symbols[BIT_N], const int16_t coeffs[BIT_N])
{
    uint32_t bad = 0;

    for (int j = 0; j < BIT_N; j++) {
        int32_t value = coeffs[j];

        bad |= (uint32_t)(value < -BIT_H_INF || value > BIT_H_INF);
        symbols[j] = (uint8_t)(value + BIT_H_INF);
    }
    return bad != 0;
}

static void join_z1_symbols(int16_t coeffs[BIT_N], const uint8_t low[BIT_N],
                            const uint8_t high[BIT_N])
{
    for (int j = 0; j < BIT_N; j++)
        coeffs[j] = (int16_t)(((int32_t)high[j] - Z0_HIGH_ZERO) * 256 + (int8_t)low[j]);
}
#define join_z0_symbols join_z1_symbols

static void join_hint_symbols(int16_t coeffs[BIT_N], const uint8_t symbols[BIT_N])
{
    for (int j = 0; j < BIT_N; j++)
        coeffs[j] = (int16_t)((int32_t)symbols[j] - BIT_H_INF);
}

int pack_sig(unsigned char *sig, const polyvecm1 *z1, const polyveck *h,
             const unsigned char *challenge)
{
    uint8_t z_high[(BIT_L + 1) * BIT_N];
    uint8_t h_symbols[BIT_K * BIT_N];
    unsigned char enc_z[RANS_TMP_BYTES], enc_h[RANS_TMP_BYTES];
    size_t z_len, h_len;
    int z0_high_is_zero = 1;
    int z0_zero_fast_path = 0;
    unsigned char *p = sig;

    memset(sig, 0, BIT_SIGNBYTES);
    memcpy(p, challenge, BIT_CHALLENGEBYTES);
    p += BIT_CHALLENGEBYTES;
    if (split_z0_symbols(p, z_high, z1->vec[0].coeffs))
        return -1;
    p += BIT_POLY_Z0_LOW_BYTES;
    for (int i = 0; i < BIT_L; i++) {
        if (split_z1_symbols(p, z_high + (i + 1) * BIT_N, z1->vec[i + 1].coeffs))
            return -1;
        p += BIT_N;
    }
    for (int i = 0; i < BIT_K; i++)
        if (hint_symbols(h_symbols + i * BIT_N, h->vec[i].coeffs))
            return -1;
    for (int i = 0; i < BIT_N; i++)
        z0_high_is_zero &= z_high[i] == Z0_HIGH_ZERO;
    if (z0_high_is_zero) {
        if (rans_encode_pair(enc_z, &z_len, z_high + BIT_N, BIT_L * BIT_N, 0, enc_h, &h_len,
                             h_symbols, h_enc, 7, sizeof(h_symbols)) != 0)
            return -1;
        z0_zero_fast_path = z_len <= RANS_LENGTH_MASK;
    }
    if (!z0_zero_fast_path && rans_encode_pair(enc_z, &z_len, z_high, sizeof(z_high), BIT_N, enc_h,
                                               &h_len, h_symbols, h_enc, 7, sizeof(h_symbols)) != 0)
        return -1;
    if (z_len > RANS_LENGTH_MASK || h_len > UINT16_MAX ||
        z_len + h_len > BIT_SIG_RANS_PAYLOAD_BYTES)
        return -1;

    store16_le(p, (uint16_t)z_len | (uint16_t)(z0_zero_fast_path ? RANS_Z0_ZERO_FLAG : 0U));
    store16_le(p + 2, (uint16_t)h_len);
    p += BIT_SIG_RANS_LENGTH_BYTES;
    memcpy(p, enc_z, z_len);
    p += z_len;
    memcpy(p, enc_h, h_len);
    return 0;
}

int unpack_sig(polyvecm1 *z1, polyveck *h, unsigned char *challenge, const unsigned char *sig)
{
    uint8_t z_high[(BIT_L + 1) * BIT_N];
    uint8_t h_symbols[BIT_K * BIT_N];
    const unsigned char *p = sig;
    size_t z_len, h_len, used;
    int z0_zero_fast_path;

    memcpy(challenge, p, BIT_CHALLENGEBYTES);
    p += BIT_CHALLENGEBYTES;
    const unsigned char *low = p;
    p += BIT_Z_LOW_BYTES;
    {
        uint16_t z_code = load16_le(p);

        z0_zero_fast_path = (z_code & RANS_Z0_ZERO_FLAG) != 0;
        z_len = z_code & RANS_LENGTH_MASK;
    }
    h_len = load16_le(p + 2);
    p += BIT_SIG_RANS_LENGTH_BYTES;
    if (z_len < 4 || h_len < 4 || z_len + h_len > BIT_SIG_RANS_PAYLOAD_BYTES)
        return -1;
    if (z0_zero_fast_path) {
        memset(z_high, Z0_HIGH_ZERO, BIT_N);
        if (rans_decode_z1_h(z_high + BIT_N, h_symbols, p, z_len, p + z_len, h_len) != 0)
            return -1;
    } else {
        if (rans_decode_pair(z_high, sizeof(z_high), BIT_N, h_symbols, sizeof(h_symbols), p, z_len,
                             p + z_len, h_len) != 0)
            return -1;
        /* All-zero z0 high parts require the flagged encoding. */
        z0_zero_fast_path = 1;
        for (int i = 0; i < BIT_N; i++)
            z0_zero_fast_path &= z_high[i] == Z0_HIGH_ZERO;
        if (z0_zero_fast_path)
            return -1;
    }
    p += z_len + h_len;

    join_z0_symbols(z1->vec[0].coeffs, low, z_high);
    low += BIT_POLY_Z0_LOW_BYTES;
    for (int i = 0; i < BIT_L; i++)
        join_z1_symbols(z1->vec[i + 1].coeffs, low + i * BIT_N, z_high + (i + 1) * BIT_N);
    for (int i = 0; i < BIT_K; i++)
        join_hint_symbols(h->vec[i].coeffs, h_symbols + i * BIT_N);

    for (int i = 0; i <= BIT_L; i++)
        for (int j = 0; j < BIT_N; j++)
            if (z1->vec[i].coeffs[j] < -BIT_RESPONSE_COEFF_BOUND ||
                z1->vec[i].coeffs[j] > BIT_RESPONSE_COEFF_BOUND)
                return -1;
    used = (size_t)(p - sig);
    for (size_t i = used; i < BIT_SIGNBYTES; i++)
        if (sig[i] != 0)
            return -1;
    return 0;
}
