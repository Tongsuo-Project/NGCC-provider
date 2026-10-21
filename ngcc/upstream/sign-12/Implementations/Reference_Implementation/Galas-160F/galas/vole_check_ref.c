/*
 * vole_check_ref.c -- portable GALAS VOLE consistency check.
 *
 * Mirrors submission/optimized/vole_check.hpp without using optimized code.
 */
#include "vole_check_ref.h"
#include "bf.h"
#include "gf2n.h"
#include <string.h>

static unsigned witness_bits(unsigned lambda) { return (5u * lambda) / 2u; }
static unsigned qs_rows(unsigned lambda) { return witness_bits(lambda) + 2u * lambda; }
static unsigned vole_check_bytes(unsigned lambda) { return lambda / 8u + 2u; }

static uint8_t bit_to_mask_byte(const uint8_t* packed, unsigned bit) {
    return (uint8_t)(0u - ((packed[bit >> 3] >> (bit & 7)) & 1u));
}

static void gf_set_zero_bytes(uint8_t* x, unsigned n) {
    memset(x, 0, n);
}

static void gf_load_chunk(const uint8_t* src, unsigned src_len,
                          unsigned lb, uint8_t* chunk) {
    memset(chunk, 0, lb);
    if (src_len > lb) src_len = lb;
    memcpy(chunk, src, src_len);
}

static void gf_horner_update(const gf_ctx* fc, gf_limb_t* state,
                             const gf_limb_t* key,
                             const gf_limb_t* input) {
    gf_limb_t t[GF_LIMBS(512)];
    gf_mul(fc, t, state, key);
    gf_add(fc, state, t, input);
}

static void column_hash(uint8_t* out, const uint8_t* challenge,
                        const uint8_t* to_hash, const galas_paramset_t* ps) {
    const unsigned lambda = ps->p.lambda;
    const unsigned lb = lambda / 8u;
    const unsigned qsr = qs_rows(lambda);
    const unsigned qsr_bytes = qsr / 8u;
    const unsigned hbytes = vole_check_bytes(lambda);
    const unsigned chunks = (qsr + lambda - 1u) / lambda;
    const unsigned gf64_chunks = (lambda + 63u) / 64u;

    gf_ctx fc;
    gf_init(&fc, lambda);
    const gf_ctx* f64 = bf_ctx_64();

    gf_limb_t matrix[4][GF_LIMBS(512)];
    gf_limb_t key_secpar[GF_LIMBS(512)];
    gf_limb_t state_secpar[GF_LIMBS(512)];
    gf_limb_t state64[GF_LIMBS(64)];
    gf_limb_t key64[GF_LIMBS(64)];

    for (unsigned i = 0; i < 4; ++i) {
        gf_from_bytes(&fc, matrix[i], challenge + (size_t)i * lb);
    }
    gf_from_bytes(&fc, key_secpar, challenge + (size_t)4u * lb);
    gf_from_bytes(f64, key64, challenge + (size_t)5u * lb);

    gf_zero(&fc, state_secpar);
    gf_zero(f64, state64);

    for (unsigned c = 0; c < chunks; ++c) {
        uint8_t chunk[GALAS_MAX_LAMBDA_BYTES];
        unsigned off = c * lb;
        unsigned take = 0;
        if (off < qsr_bytes) take = qsr_bytes - off;
        if (take > lb) take = lb;
        gf_load_chunk(to_hash + off, take, lb, chunk);

        gf_limb_t in_secpar[GF_LIMBS(512)];
        gf_from_bytes(&fc, in_secpar, chunk);
        gf_horner_update(&fc, state_secpar, key_secpar, in_secpar);

        for (unsigned j = 0; j < gf64_chunks; ++j) {
            uint8_t limb[8];
            unsigned boff = j * 8u;
            unsigned btake = 0;
            memset(limb, 0, sizeof(limb));
            if (boff < lb) {
                btake = lb - boff;
                if (btake > sizeof(limb)) btake = sizeof(limb);
                memcpy(limb, chunk + boff, btake);
            }

            gf_limb_t in64[GF_LIMBS(64)];
            gf_from_bytes(f64, in64, limb);
            gf_horner_update(f64, state64, key64, in64);
        }
    }

    uint8_t state64_bytes[GALAS_MAX_LAMBDA_BYTES];
    gf_set_zero_bytes(state64_bytes, lb);
    gf_to_bytes(f64, state64_bytes, state64);

    gf_limb_t state64_ext[GF_LIMBS(512)];
    gf_from_bytes(&fc, state64_ext, state64_bytes);

    gf_limb_t mapped0[GF_LIMBS(512)], mapped1[GF_LIMBS(512)];
    gf_limb_t p0[GF_LIMBS(512)], p1[GF_LIMBS(512)];
    gf_mul(&fc, p0, matrix[0], state_secpar);
    gf_mul(&fc, p1, matrix[1], state64_ext);
    gf_add(&fc, mapped0, p0, p1);
    gf_mul(&fc, p0, matrix[2], state_secpar);
    gf_mul(&fc, p1, matrix[3], state64_ext);
    gf_add(&fc, mapped1, p0, p1);

    uint8_t first[GALAS_MAX_LAMBDA_BYTES];
    uint8_t second[GALAS_MAX_LAMBDA_BYTES];
    gf_to_bytes(&fc, first, mapped0);
    gf_to_bytes(&fc, second, mapped1);

    memcpy(out, first, lb);
    memcpy(out + lb, second, hbytes - lb);
    for (unsigned i = 0; i < hbytes; ++i) {
        out[i] ^= to_hash[qsr_bytes + i];
    }
}

void galas_vole_check_sender(uint8_t* proof, galas_H2_ctx* h2,
                             const uint8_t* challenge,
                             const uint8_t* u, uint8_t* const* v,
                             const galas_paramset_t* ps) {
    const unsigned lambda = ps->p.lambda;
    const unsigned hbytes = vole_check_bytes(lambda);
    const unsigned qsr_bytes = qs_rows(lambda) / 8u;
    const unsigned delta_bits = lambda - ps->p.w_grind;

    column_hash(proof, challenge, u, ps);
    galas_H2_update(h2, proof, hbytes);

    uint8_t h[GALAS_MAX_LAMBDA_BYTES + 2];
    for (unsigned col = 0; col < delta_bits; ++col) {
        column_hash(h, challenge, v[col], ps);
        galas_H2_update(h2, h, hbytes);
    }
    for (unsigned col = delta_bits; col < lambda; ++col) {
        galas_H2_update(h2, v[col] + qsr_bytes, hbytes);
    }
}

void galas_vole_check_receiver(galas_H2_ctx* h2, const uint8_t* challenge,
                               const uint8_t* proof, uint8_t* const* q,
                               const uint8_t* delta,
                               const galas_paramset_t* ps) {
    const unsigned lambda = ps->p.lambda;
    const unsigned lb = lambda / 8u;
    const unsigned hbytes = vole_check_bytes(lambda);
    const unsigned qsr_bytes = qs_rows(lambda) / 8u;
    const unsigned delta_bits = lambda - ps->p.w_grind;

    uint8_t h[GALAS_MAX_LAMBDA_BYTES + 2];
    for (unsigned col = 0; col < delta_bits; ++col) {
        column_hash(h, challenge, q[col], ps);
        if (bit_to_mask_byte(delta, col)) {
            for (unsigned j = 0; j < hbytes; ++j) {
                h[j] ^= proof[j];
            }
        }
        galas_H2_update(h2, h, hbytes);
    }
    for (unsigned col = delta_bits; col < lambda; ++col) {
        galas_H2_update(h2, q[col] + qsr_bytes, hbytes);
    }
    (void)lb;
}
