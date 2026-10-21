/*
 * universal_hashing.c — generic VOLEHash / ZKHash / LeafHash for GALAS.
 *
 * Faithful port of RefCodes/ref-FAEST/faest_128f/universal_hashing.c, with the
 * per-lambda specializations removed in favor of bf_ctx-driven field ops. The
 * algebra is identical; only the field width is parameterized.
 *
 *   compute_h1: Horner over GF(2^64) using the 64-bit halves of x (high end).
 *   vole_hash:  h0 = Horner over GF(2^lambda) with base s;  h1 = compute_h1;
 *               out = (r0*h0 + r1*h1) || (r2*h0 + r3*h1)[low B bits] XOR x1.
 *   zk_hash:    h0,h1 accumulators updated by Horner; finalize mixes r0,r1.
 *   leaf_hash:  u*x0 (unreduced, 2*lambda) + x1.
 */
#include "universal_hashing.h"
#include "bf.h"
#include <string.h>
#include <stdlib.h>

static const gf_ctx* fc_for(unsigned lambda) {
    switch (lambda) {
        case 160: return bf_ctx_160();
        case 256: return bf_ctx_256();
        case 384: return bf_ctx_384();
        case 512: return bf_ctx_512();
        default:  return NULL;
    }
}

/* compute_h1: GF(2^64) Horner over the top 64-bit limbs of x, base b_t.
   Matches FAEST compute_h1. Returns a 64-bit value. */
static uint64_t compute_h1_g(uint64_t b_t, const uint8_t* x, unsigned lambda, unsigned ell) {
    unsigned lb = lambda / 8;
    unsigned length_lambda = (ell + 3 * lambda - 1) / lambda;
    /* tmp = the last (partial) lambda-bit chunk of x, zero-extended to lb bytes.
       FAEST uses a MAX_LAMBDA_BYTES buffer; we size to lb (<= 64). */
    uint8_t tmp[64];
    memset(tmp, 0, sizeof(tmp));
    unsigned copy = ((ell + lambda) % lambda == 0) ? lb : ((ell + lambda) % lambda) / 8;
    if (copy > sizeof(tmp)) copy = sizeof(tmp);
    if (copy > 0) memcpy(tmp, x + (length_lambda - 1) * lb, copy);

    const gf_ctx* fc64 = bf_ctx_64();
    uint64_t h1 = 0;
    gf_limb_t running[1] = {1};
    unsigned i = 0;
    /* FAEST compute_h1: process tmp[] in 64-bit words high-to-low. FAEST reads
       tmp[lambda_bytes - i - 8] for i in 0,8,16,... which goes negative once
       i > lambda_bytes - 8; it relies on reading stack padding (zero from the
       ={0} init of a larger buffer). We replicate by zero-padding tmp to a
       multiple of 8 and reading safely. */
    unsigned tmp_padded = ((lb + 7) / 8) * 8;   /* round lb up to 8 */
    /* first loop: 8-byte words over tmp (and zero padding beyond copy) */
    for (; i < tmp_padded; i += 8) {
        gf_limb_t word[1]; word[0] = 0;
        int base = (int)tmp_padded - (int)i - 8;
        for (unsigned b = 0; b < 8; ++b) {
            int idx = base + (int)b;
            uint8_t byte = (idx >= 0 && (unsigned)idx < sizeof(tmp)) ? tmp[idx] : 0;
            word[0] |= ((uint64_t)byte) << (8 * b);
        }
        gf_limb_t prod[1]; gf_mul(fc64, prod, running, word);
        h1 ^= prod[0];
        gf_limb_t nr[1]; gf_mul(fc64, nr, running, &b_t);
        running[0] = nr[0];
    }
    /* second loop: 8-byte words over x (high-to-low), reading x[l_end - i - 8] */
    unsigned xbytes = length_lambda * lb;
    for (; i < xbytes; i += 8) {
        gf_limb_t word[1]; word[0] = 0;
        int base = (int)xbytes - (int)i - 8;
        for (unsigned b = 0; b < 8; ++b) {
            int idx = base + (int)b;
            uint8_t byte = (idx >= 0) ? x[idx] : 0;
            word[0] |= ((uint64_t)byte) << (8 * b);
        }
        gf_limb_t prod[1]; gf_mul(fc64, prod, running, word);
        h1 ^= prod[0];
        gf_limb_t nr[1]; gf_mul(fc64, nr, running, &b_t);
        running[0] = nr[0];
    }
    return h1;
}

void galas_vole_hash(uint8_t* h, const uint8_t* sd, const uint8_t* x,
                     unsigned ell, unsigned lambda) {
    const gf_ctx* fc = fc_for(lambda);
    unsigned lb = lambda / 8;
    /* sd layout: r0,r1,r2,r3 (each lb), s (lb), t (8) */
    const uint8_t* r0 = sd;
    const uint8_t* r1 = sd + 1 * lb;
    const uint8_t* r2 = sd + 2 * lb;
    const uint8_t* r3 = sd + 3 * lb;
    const uint8_t* s  = sd + 4 * lb;
    const uint8_t* t  = sd + 5 * lb;
    const uint8_t* x1 = x + (ell + 2 * lb * 8) / 8;

    unsigned length_lambda = (ell + 3 * lambda - 1) / lambda;

    /* h0 = Horner over GF(2^lambda) with base s */
    uint8_t tmp[64] = {0};
    unsigned copy = ((ell + lambda) % lambda == 0) ? lb : ((ell + lambda) % lambda) / 8;
    memcpy(tmp, x + (length_lambda - 1) * lb, copy);
    gf_limb_t h0[GF_LIMBS(512)]; gf_from_bytes(fc, h0, tmp);
    gf_limb_t bs[GF_LIMBS(512)]; gf_from_bytes(fc, bs, s);
    gf_limb_t running[GF_LIMBS(512)]; gf_copy(fc, running, bs);
    for (unsigned i = 1; i < length_lambda; ++i) {
        gf_limb_t word[GF_LIMBS(512)];
        memset(word, 0, sizeof(word));
        memcpy(word, x + (length_lambda - 1 - i) * lb, lb);
        gf_limb_t prod[GF_LIMBS(512)]; gf_mul(fc, prod, running, word);
        gf_add(fc, h0, h0, prod);
        gf_limb_t nr[GF_LIMBS(512)]; gf_mul(fc, nr, running, bs);
        gf_copy(fc, running, nr);
    }

    uint64_t h1 = compute_h1_g(*(const uint64_t*)t, x, lambda, ell);

    /* h2 = r0*h0 + r1*h1 (lambda bits); h3 = r2*h0 + r3*h1 (lambda bits) */
    gf_limb_t R0[GF_LIMBS(512)], R1[GF_LIMBS(512)], R2[GF_LIMBS(512)], R3[GF_LIMBS(512)];
    gf_from_bytes(fc, R0, r0); gf_from_bytes(fc, R1, r1);
    gf_from_bytes(fc, R2, r2); gf_from_bytes(fc, R3, r3);
    gf_limb_t h1l[GF_LIMBS(512)]; memset(h1l, 0, sizeof(h1l)); h1l[0] = h1;

    gf_limb_t h2[GF_LIMBS(512)], t0[GF_LIMBS(512)], t1[GF_LIMBS(512)];
    gf_mul(fc, t0, R0, h0); gf_mul(fc, t1, R1, h1l); gf_add(fc, h2, t0, t1);
    gf_limb_t h3[GF_LIMBS(512)];
    gf_mul(fc, t0, R2, h0); gf_mul(fc, t1, R3, h1l); gf_add(fc, h3, t0, t1);

    /* output: h2 (lb bytes) || h3 low B bytes, then XOR x1 */
    gf_to_bytes(fc, h, h2);
    gf_to_bytes(fc, tmp, h3);
    memcpy(h + lb, tmp, GALAS_UNIVERSAL_HASH_B);
    for (unsigned i = 0; i < lb + GALAS_UNIVERSAL_HASH_B; ++i) h[i] ^= x1[i];
}

void galas_zk_hash_init(galas_zk_hash_ctx* ctx, unsigned lambda, const uint8_t* sd) {
    ctx->lambda = lambda;
    ctx->fc = fc_for(lambda);
    unsigned lb = lambda / 8;
    gf_zero(ctx->fc, ctx->h0);
    ctx->h1[0] = 0;
    const uint8_t* s = sd + 2 * lb;   /* FAEST zk_hash: s at offset 2*lambda */
    const uint8_t* t = sd + 3 * lb;
    gf_from_bytes(ctx->fc, ctx->s, s);
    ctx->t[0] = *(const uint64_t*)t;
    ctx->sd = sd;
}

void galas_zk_hash_update(galas_zk_hash_ctx* ctx, const gf_limb_t* v) {
    /* h0 = s*h0 + v ; h1 = t*h1 + v (over GF(2^64)) */
    gf_limb_t prod[GF_LIMBS(512)];
    gf_mul(ctx->fc, prod, ctx->h0, ctx->s);
    gf_add(ctx->fc, ctx->h0, prod, v);
    gf_limb_t v64[1]; v64[0] = v[0];
    gf_limb_t p64[1]; gf_mul(bf_ctx_64(), p64, ctx->h1, ctx->t);
    ctx->h1[0] = p64[0] ^ v64[0];
}

void galas_zk_hash_final(uint8_t* h, galas_zk_hash_ctx* ctx, const gf_limb_t* x1) {
    unsigned lb = ctx->lambda / 8;
    const uint8_t* r0 = ctx->sd;
    const uint8_t* r1 = ctx->sd + lb;
    gf_limb_t R0[GF_LIMBS(512)], R1[GF_LIMBS(512)];
    gf_from_bytes(ctx->fc, R0, r0); gf_from_bytes(ctx->fc, R1, r1);
    gf_limb_t h1l[GF_LIMBS(512)]; memset(h1l, 0, sizeof(h1l)); h1l[0] = ctx->h1[0];
    gf_limb_t a[GF_LIMBS(512)], b[GF_LIMBS(512)], sum[GF_LIMBS(512)];
    gf_mul(ctx->fc, a, R0, ctx->h0);
    gf_mul(ctx->fc, b, R1, h1l);
    gf_add(ctx->fc, sum, a, b);
    gf_add(ctx->fc, sum, sum, x1);
    gf_to_bytes(ctx->fc, h, sum);
}

void galas_leaf_hash(uint8_t* h, const uint8_t* u, const uint8_t* x0, const uint8_t* x1,
                     unsigned lambda) {
    const gf_ctx* fc = fc_for(lambda);
    unsigned lb = lambda / 8;
    gf_limb_t X0[GF_LIMBS(512)]; gf_from_bytes(fc, X0, x0);
    /* u is 2*lambda bits; product u*x0 is up to 3*lambda bits. */
    unsigned l2 = (2 * lambda + 63) / 64;
    unsigned lout = (3 * lambda + 63) / 64;
    gf_limb_t U[GF_LIMBS(512) * 3];   /* oversized for safety */
    memset(U, 0, sizeof(U));
    for (unsigned i = 0; i < 2 * lb; ++i)
        U[i / 8] |= ((uint64_t)u[i]) << (8 * (i % 8));
    gf_limb_t prod[GF_LIMBS(512) * 3];
    for (unsigned i = 0; i < lout; ++i) prod[i] = 0;
    gf_limb_t acc[GF_LIMBS(512) * 3];
    for (unsigned i = 0; i < l2; ++i) acc[i] = U[i];
    for (unsigned i = l2; i < lout; ++i) acc[i] = 0;
    for (unsigned k = 0; k < lambda; ++k) {
        if ((X0[k / 64] >> (k % 64)) & 1) {
            for (unsigned i = 0; i < lout; ++i) prod[i] ^= acc[i];
        }
        uint64_t carry = 0;
        for (unsigned i = 0; i < lout; ++i) {
            uint64_t nx = (acc[i] >> 63) & 1;
            acc[i] = (acc[i] << 1) | carry;
            carry = nx;
        }
    }
    /* add x1 (lambda bits, zero-extended); output is 2*lambda bits. */
    gf_limb_t X1[GF_LIMBS(512) * 3]; memset(X1, 0, sizeof(X1));
    for (unsigned i = 0; i < lb; ++i) X1[i / 8] |= ((uint64_t)x1[i]) << (8 * (i % 8));
    unsigned lout2 = (2 * lambda + 63) / 64;
    gf_limb_t out[GF_LIMBS(512) * 3];
    for (unsigned i = 0; i < lout2; ++i) out[i] = prod[i] ^ X1[i];
    for (unsigned i = 0; i < 2 * lb; ++i)
        h[i] = (uint8_t)((out[i / 8] >> (8 * (i % 8))) & 0xff);
}
