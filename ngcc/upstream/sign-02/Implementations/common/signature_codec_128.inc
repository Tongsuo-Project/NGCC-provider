/* Copyright (c) 2026 Hang Zhang. */

#include "rans_decode.h"

#define RANS_SCALE_BITS 16
#define RANS_SCALE (1U << RANS_SCALE_BITS)
#define RANS_BYTE_L (1U << 23)
#define RANS_TMP_BYTES (((BIT_L + 1) * BIT_N * RANS_SCALE_BITS + 7) / 8 + 8)
#define RANS_Z0_ZERO_FLAG 0x8000U
#define RANS_LENGTH_MASK 0x7FFFU
/* Symbol origin of the full-support frequency table. */
#define Z0_HIGH_BOUND ((int)(sizeof(z_high_freq) / sizeof(z_high_freq[0]) / 2))
#define Z0_HIGH_SYMBOLS (2 * Z0_HIGH_BOUND + 1)
#define Z0_HIGH_ZERO Z0_HIGH_BOUND

typedef struct {
    uint32_t x_max;
    uint32_t rcp_freq;
    uint32_t bias;
    uint16_t cmpl_freq;
    uint8_t rcp_shift;
} rans_enc_symbol;

#include "rans_model.h"

static int rans_encode(unsigned char *out, size_t *out_len, const uint8_t *symbols, size_t count,
                       const rans_enc_symbol *model, unsigned int nsymbols)
{
    unsigned char tmp[RANS_TMP_BYTES];
    unsigned char *ptr = tmp + sizeof(tmp);
    uint32_t state = RANS_BYTE_L;

    for (size_t i = count; i > 0; i--) {
        unsigned int symbol = symbols[i - 1];
        const rans_enc_symbol *sym;
        uint32_t x;
        uint32_t q;

        if (symbol >= nsymbols)
            return -1;
        sym = &model[symbol];
        x = state;
        while (x >= sym->x_max) {
            if (ptr == tmp)
                return -1;
            *--ptr = (uint8_t)x;
            x >>= 8;
        }
        q = (uint32_t)(((uint64_t)x * sym->rcp_freq) >> 32) >> sym->rcp_shift;
        state = x + sym->bias + q * sym->cmpl_freq;
    }
    if ((size_t)(ptr - tmp) < 4)
        return -1;
    ptr -= 4;
    store32_le_local(ptr, state);
    *out_len = (size_t)(tmp + sizeof(tmp) - ptr);
    memcpy(out, ptr, *out_len);
    return 0;
}

static int rans_encode_z(unsigned char *out, size_t *out_len, const uint8_t *symbols, size_t count,
                         size_t z0_count)
{
    unsigned char tmp[RANS_TMP_BYTES];
    unsigned char *ptr = tmp + sizeof(tmp);
    uint32_t state = RANS_BYTE_L;

    for (size_t i = count; i > 0; i--) {
        size_t index = i - 1;
        const rans_enc_symbol *model = index < z0_count ? z0_high_enc : z_high_enc;
        unsigned int nsymbols = Z0_HIGH_SYMBOLS;
        unsigned int symbol = symbols[index];
        const rans_enc_symbol *sym;
        uint32_t x;
        uint32_t q;

        if (symbol >= nsymbols)
            return -1;
        sym = &model[symbol];
        x = state;
        while (x >= sym->x_max) {
            if (ptr == tmp)
                return -1;
            *--ptr = (uint8_t)x;
            x >>= 8;
        }
        q = (uint32_t)(((uint64_t)x * sym->rcp_freq) >> 32) >> sym->rcp_shift;
        state = x + sym->bias + q * sym->cmpl_freq;
    }
    if ((size_t)(ptr - tmp) < 4)
        return -1;
    ptr -= 4;
    store32_le_local(ptr, state);
    *out_len = (size_t)(tmp + sizeof(tmp) - ptr);
    memcpy(out, ptr, *out_len);
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
    z_state = load32_le_local(z_ptr);
    h_state = load32_le_local(h_ptr);
    z_ptr += 4;
    h_ptr += 4;
    if (z_state < RANS_BYTE_L || z_state >= (RANS_BYTE_L << 8) || h_state < RANS_BYTE_L ||
        h_state >= (RANS_BYTE_L << 8))
        return -1;

    for (size_t i = 0; i < z_count || i < h_count; i++) {
        if (i < z_count) {
            const uint16_t *freq = i < z0_count ? z0_high_freq : z_high_freq;
            uint32_t slot = z_state & (RANS_SCALE - 1);
            uint32_t start = 0;
            uint32_t frequency;
            unsigned int symbol;

            symbol = rans_find_symbol_fast(slot, freq, z_high_fast, &start, &frequency);
            z_symbols[i] = (uint8_t)symbol;
            z_state = frequency * (z_state >> RANS_SCALE_BITS) + slot - start;
            while (z_state < RANS_BYTE_L && z_ptr < z_end)
                z_state = (z_state << 8) | *z_ptr++;
        }
        if (i < h_count) {
            uint32_t slot = h_state & (RANS_SCALE - 1);
            uint32_t start = 0;
            uint32_t frequency;
            unsigned int symbol;

            symbol = rans_find_symbol_fast(slot, h_freq, h_fast, &start, &frequency);
            h_symbols[i] = (uint8_t)symbol;
            h_state = frequency * (h_state >> RANS_SCALE_BITS) + slot - start;
            while (h_state < RANS_BYTE_L && h_ptr < h_end)
                h_state = (h_state << 8) | *h_ptr++;
        }
    }
    return z_state == RANS_BYTE_L && h_state == RANS_BYTE_L && z_ptr == z_end && h_ptr == h_end
               ? 0
               : -1;
}

/* Every response coefficient uses the same centered byte decomposition. */
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

static int join_z1_symbols(int16_t coeffs[BIT_N], const uint8_t low[BIT_N],
                           const uint8_t high[BIT_N])
{
    uint32_t bad = 0;
    for (int j = 0; j < BIT_N; j++) {
        int32_t z = ((int32_t)high[j] - Z0_HIGH_ZERO) * 256 + (int8_t)low[j];
        bad |= (uint32_t)(z < -BIT_RESPONSE_COEFF_BOUND || z > BIT_RESPONSE_COEFF_BOUND);
        coeffs[j] = (int16_t)z;
    }
    return bad != 0;
}

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
    size_t z_len = 0, h_len = 0;
    int z0_high_is_zero = 1;
    int z0_zero_fast_path = 0;
    unsigned char *p = sig;

    memset(sig, 0, BIT_SIGNBYTES);
    memcpy(p, challenge, BIT_CHALLENGEBYTES);
    p += BIT_CHALLENGEBYTES;
    if (split_z1_symbols(p, z_high, z1->vec[0].coeffs)) {
        TRACE_RANS(0);
        return -1;
    }
    p += BIT_POLY_Z0_LOW_BYTES;
    for (int i = 0; i < BIT_L; i++) {
        if (split_z1_symbols(p, z_high + (i + 1) * BIT_N, z1->vec[i + 1].coeffs)) {
            TRACE_RANS(0);
            return -1;
        }
        p += BIT_N;
    }
    for (int i = 0; i < BIT_K; i++)
        if (hint_symbols(h_symbols + i * BIT_N, h->vec[i].coeffs)) {
            TRACE_RANS(0);
            return -1;
        }
    for (int i = 0; i < BIT_N; i++)
        z0_high_is_zero &= z_high[i] == Z0_HIGH_ZERO;
    if (z0_high_is_zero) {
        if (rans_encode(enc_z, &z_len, z_high + BIT_N, BIT_L * BIT_N, z_high_enc,
                        Z0_HIGH_SYMBOLS) != 0) {
            TRACE_RANS(0);
            return -1;
        }
        z0_zero_fast_path = z_len <= RANS_LENGTH_MASK;
    }
    if (!z0_zero_fast_path && rans_encode_z(enc_z, &z_len, z_high, sizeof(z_high), BIT_N) != 0) {
        TRACE_RANS(0);
        return -1;
    }
    if (rans_encode(enc_h, &h_len, h_symbols, sizeof(h_symbols), h_enc, 7) != 0) {
        TRACE_RANS(0);
        return -1;
    }
    TRACE_RANS(1);
    if (z_len > RANS_LENGTH_MASK || h_len > 255 || z_len + h_len > BIT_SIG_RANS_PAYLOAD_BYTES)
        return -1;

    store16_le(p, (uint16_t)z_len | (uint16_t)(z0_zero_fast_path ? RANS_Z0_ZERO_FLAG : 0U));
    p += 2;
    *p++ = (uint8_t)h_len;
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
    const unsigned char *z0_low = p;
    const unsigned char *z1_low = p + BIT_POLY_Z0_LOW_BYTES;
    p += BIT_Z_LOW_BYTES;
    uint16_t z_code = load16_le(p);
    z0_zero_fast_path = (z_code & RANS_Z0_ZERO_FLAG) != 0;
    z_len = z_code & RANS_LENGTH_MASK;
    p += 2;
    h_len = *p++;
    if (z_len < 4 || h_len < 4 || z_len > BIT_SIG_RANS_PAYLOAD_BYTES ||
        h_len > BIT_SIG_RANS_PAYLOAD_BYTES - z_len)
        return -1;
    if (z0_zero_fast_path) {
        memset(z_high, Z0_HIGH_ZERO, BIT_N);
        if (rans_decode_pair(z_high + BIT_N, BIT_L * BIT_N, 0, h_symbols, sizeof(h_symbols), p,
                             z_len, p + z_len, h_len) != 0)
            return -1;
    } else {
        if (rans_decode_pair(z_high, sizeof(z_high), BIT_N, h_symbols, sizeof(h_symbols), p, z_len,
                             p + z_len, h_len) != 0)
            return -1;
        z0_zero_fast_path = 1;
        for (int i = 0; i < BIT_N; i++)
            z0_zero_fast_path &= z_high[i] == Z0_HIGH_ZERO;
        if (z0_zero_fast_path)
            return -1;
    }
    p += z_len + h_len;

    if (join_z1_symbols(z1->vec[0].coeffs, z0_low, z_high))
        return -1;
    for (int i = 0; i < BIT_L; i++)
        if (join_z1_symbols(z1->vec[i + 1].coeffs, z1_low + i * BIT_N, z_high + (i + 1) * BIT_N))
            return -1;
    for (int i = 0; i < BIT_K; i++)
        join_hint_symbols(h->vec[i].coeffs, h_symbols + i * BIT_N);

    used = (size_t)(p - sig);
    for (size_t i = used; i < BIT_SIGNBYTES; i++)
        if (sig[i] != 0)
            return -1;
    return 0;
}
