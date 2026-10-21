/*
 * challenge.c - chall_3 decode / grinding check.
 *
 * The first lambda-w_grind bits of chall_3 are parsed as the concatenation of
 * tau little-endian leaf indices: the first tau1 indices use k bits and the
 * remaining tau0 indices use k-1 bits.  The final w_grind bits must be zero.
 */
#include "challenge.h"

static inline uint8_t get_bit(const uint8_t* s, unsigned i) {
    return (uint8_t)((s[i >> 3] >> (i & 7)) & 1u);
}

static inline void set_bit(uint8_t* s, unsigned i, uint8_t bit) {
    if (bit) s[i >> 3] |= (uint8_t)(1u << (i & 7));
    else s[i >> 3] &= (uint8_t)~(1u << (i & 7));
}

bool galas_check_chall_3(const uint8_t* chall_3, unsigned lambda, unsigned w_grind) {
    if (w_grind == 0) return true;
    if (w_grind > lambda) return false;

    for (unsigned bit = lambda - w_grind; bit < lambda; ++bit) {
        if (get_bit(chall_3, bit)) return false;
    }
    return true;
}

bool galas_decode_all_chall_3(uint16_t* i_delta, const uint8_t* chall_3,
                              const galas_paramset_t* ps) {
    for (unsigned i = 0; i < ps->p.tau; ++i) {
        unsigned lo;
        unsigned hi;
        if (i < ps->p.tau1) {
            lo = i * ps->p.k;
            hi = (i + 1u) * ps->p.k;
        } else {
            const unsigned t = i - ps->p.tau1;
            lo = ps->p.tau1 * ps->p.k + t * (ps->p.k - 1u);
            hi = ps->p.tau1 * ps->p.k + (t + 1u) * (ps->p.k - 1u);
        }

        uint8_t tmp[2] = {0, 0};
        for (unsigned j = lo; j < hi; ++j) {
            set_bit(tmp, j - lo, get_bit(chall_3, j));
        }
        i_delta[i] = (uint16_t)tmp[0] | ((uint16_t)tmp[1] << 8);
    }
    return true;
}
