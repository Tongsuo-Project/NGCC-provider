/*
 * gf2n.h — portable GF(2^n) arithmetic for the GALAS reference implementation.
 *
 * Field elements are stored as little-endian arrays of 64-bit limbs
 * (limb 0 = bits 0..63), matching the encoding used by galas_params_<n>.h.
 * The reduction polynomial phi is fixed per n and supplied via the
 * GALAS_<N>_FIELD_MODULUS macro (see gf2n.c, which hard-codes the five
 * supported n).
 *
 * Supported n: 128, 160, 256, 384, 512.
 */
#ifndef GALAS_GF2N_H
#define GALAS_GF2N_H

#include <stdint.h>
#include <stddef.h>

/* Number of 64-bit limbs needed for an n-bit element. */
#define GF_LIMBS(n) (((n) + 63) / 64)

typedef uint64_t gf_limb_t;

/* Field context: holds n, the reduction polynomial, and derived sizes. */
typedef struct {
    unsigned n;            /* field extension degree (128/160/256/384/512) */
    unsigned nbits;        /* == n */
    unsigned nbytes;       /* (n+7)/8 */
    unsigned nlimbs;       /* GF_LIMBS(n) */
    /* reduction polynomial phi as a limb array of length nlimbs+1
       (the top limb holds bit n, which is always set). */
    gf_limb_t phi[GF_LIMBS(512) + 2];
} gf_ctx;

/* Initialize a context for the given n. Returns 0 on success, -1 if n is
   unsupported. */
int gf_init(gf_ctx* ctx, unsigned n);

/* out <- 0 */
void gf_zero(const gf_ctx* ctx, gf_limb_t* out);
/* out <- a */
void gf_copy(const gf_ctx* ctx, gf_limb_t* out, const gf_limb_t* a);
/* returns 1 if a == 0 */
int gf_is_zero(const gf_ctx* ctx, const gf_limb_t* a);

/* Load/store from a little-endian byte array of length ctx->nbytes. */
void gf_from_bytes(const gf_ctx* ctx, gf_limb_t* out, const uint8_t* in);
void gf_to_bytes(const gf_ctx* ctx, uint8_t* out, const gf_limb_t* a);

/* out <- a + b  (== a XOR b in GF(2)) */
void gf_add(const gf_ctx* ctx, gf_limb_t* out, const gf_limb_t* a, const gf_limb_t* b);
/* out <- a XOR b  (in place variant: a ^= b) */
void gf_xor(const gf_ctx* ctx, gf_limb_t* a, const gf_limb_t* b);

/* out <- a * b mod phi */
void gf_mul(const gf_ctx* ctx, gf_limb_t* out, const gf_limb_t* a, const gf_limb_t* b);
/* out <- a^2 (squaring is linear: interleave zeros then reduce) */
void gf_sqr(const gf_ctx* ctx, gf_limb_t* out, const gf_limb_t* a);

/* out <- a^(2^j) by j squarings */
void gf_frobenius(const gf_ctx* ctx, gf_limb_t* out, const gf_limb_t* a, unsigned j);

/* out <- a^{-1} via a^(2^n - 2), addity-extend-and-add chain.
   If a == 0, out <- 0 (by convention, matching galas_qs_gadgets.hpp). */
void gf_inv(const gf_ctx* ctx, gf_limb_t* out, const gf_limb_t* a);

#endif /* GALAS_GF2N_H */
