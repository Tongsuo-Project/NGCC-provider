/* Copyright (c) 2026 Hang Zhang. */
#ifndef BIT_RANS_DECODE_TEST_H
#define BIT_RANS_DECODE_TEST_H
#include <stdint.h>
#include "params.h"
#include "rans_decode.h"
#define RANS_SCALE_BITS 16
typedef struct {
    uint32_t x_max;
    uint32_t rcp_freq;
    uint32_t bias;
    uint16_t cmpl_freq;
    uint8_t rcp_shift;
} rans_enc_symbol;
#if BIT_SECURITY_BITS == 128
#include "rans_model_128.h"
#elif BIT_SECURITY_BITS == 256
#include "rans_model_256.h"
#else
#include "rans_model_512.h"
#endif
static int test_rans_model(const uint16_t *freq, const uint16_t *cumulative,
                           const uint8_t *lookup, unsigned int count)
{
    uint32_t base = 0;
    for (unsigned int symbol = 0; symbol < count; symbol++) {
        if (freq[symbol] == 0 || cumulative[symbol] != base)
            return 1;
        for (uint32_t slot = base; slot < base + freq[symbol]; slot++) {
            uint32_t start = UINT32_MAX;
            unsigned int actual = rans_find_symbol(slot, freq, cumulative, lookup, &start);
            if (actual != symbol || start != base)
                return 1;
        }
        base += freq[symbol];
    }
    return base != 65536;
}
static int test_rans_lookup_base(void)
{
    return test_rans_model(z_high_freq, z_high_start, z_high_lookup,
                           sizeof(z_high_freq) / sizeof(z_high_freq[0])) ||
           test_rans_model(h_freq, h_start, h_lookup, sizeof(h_freq) / sizeof(h_freq[0]));
}
#if BIT_SECURITY_BITS != 512
static int test_rans_fast_model(const uint16_t *freq, const uint16_t *cumulative,
                               const uint8_t *lookup, const uint64_t *fast)
{
    for (uint32_t slot = 0; slot < 65536; slot++) {
        uint32_t start, fast_start, frequency;
        unsigned int symbol = rans_find_symbol(slot, freq, cumulative, lookup, &start);
        unsigned int actual = rans_find_symbol_fast(slot, freq, fast, &fast_start, &frequency);
        if (actual != symbol || fast_start != start || frequency != freq[symbol])
            return 1;
    }
    return 0;
}
static int test_rans_fast_lookup(void)
{
    return test_rans_fast_model(z_high_freq, z_high_start, z_high_lookup, z_high_fast) ||
           test_rans_fast_model(h_freq, h_start, h_lookup, h_fast);
}
#else
static int test_rans_fast_lookup(void) { return 0; }
#endif
static int test_rans_lookup(void)
{
    return test_rans_lookup_base() || test_rans_fast_lookup();
}
#endif
