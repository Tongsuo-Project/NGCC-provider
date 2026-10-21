/*
 * universal_hashing.h — VOLEHash / ZKHash / LeafHash for GALAS QuickSilver.
 *
 * Generic port of RefCodes/ref-FAEST/faest_128f/universal_hashing.c, freed
 * from the hard-coded bf128/bf192/bf256 specializations: the field size is
 * chosen at runtime via a galas_hash_ctx that carries the bf contexts.
 *
 * The "small field" used by the t-side accumulator is always GF(2^64)
 * (UNIVERSAL_HASH_B_BITS = 16, B = 2 bytes), matching FAEST.
 *
 * Three hash families:
 *   vole_hash  : the VOLE consistency hash (binds the prover's u/V to a single
 *                transcript). h = r0*h0 + r1*h1 (lambda bits) || r2*h0 + r3*h1
 *                (B bits) XOR x1. Operates over a lambda-bit VOLE of length ell.
 *   zk_hash    : the QuickSilver batching hash (Horner over authenticated field
 *                elements). init/update/finalize.
 *   leaf_hash  : the BAVC leaf commitment's OWF-bound hash = u*x0 + x1, where
 *                u is in the 2*lambda extension ring and x0,x1 are lambda-bit.
 */
#ifndef GALAS_UNIVERSAL_HASHING_H
#define GALAS_UNIVERSAL_HASHING_H

#include <stdint.h>
#include <stddef.h>
#include "bf.h"

#define GALAS_UNIVERSAL_HASH_B_BITS 16
#define GALAS_UNIVERSAL_HASH_B (GALAS_UNIVERSAL_HASH_B_BITS / 8)

/* Hash context: carries the lambda-bit field ctx and the seed-derived keys. */
typedef struct {
    unsigned lambda;          /* security parameter / field size */
    const gf_ctx* fc;         /* GF(2^lambda) ctx (the VOLE auth field) */
    /* seed sd layout (6*lambda bits): r0,r1,r2,r3 (lambda), s (lambda), t (64) */
    uint8_t* sd;              /* 6 * lambda/8 bytes, owned by caller */
} galas_hash_ctx;

/* h (out): lambda bits || B bits. sd: 6*lambda/8 bytes seed. x: the VOLE
   vector ((ell + 2*lambda) bits, i.e. ell/8 + 2*lambda/8 bytes). ell in bits. */
void galas_vole_hash(uint8_t* h, const uint8_t* sd, const uint8_t* x,
                     unsigned ell, unsigned lambda);

/* ZK hash (QuickSilver batching). Accumulates lambda-bit field elements. */
typedef struct {
    unsigned lambda;
    const gf_ctx* fc;
    gf_limb_t h0[GF_LIMBS(512)];   /* lambda-bit accumulator */
    uint64_t  h1[1];                /* GF(2^64) accumulator (t-side) */
    gf_limb_t s[GF_LIMBS(512)];    /* lambda-bit Horner base */
    uint64_t  t[1];                 /* GF(2^64) Horner base */
    const uint8_t* sd;             /* r0,r1 at offsets 0, lambda */
} galas_zk_hash_ctx;

void galas_zk_hash_init(galas_zk_hash_ctx* ctx, unsigned lambda, const uint8_t* sd);
void galas_zk_hash_update(galas_zk_hash_ctx* ctx, const gf_limb_t* v /* lambda bits */);
/* finalize: h (lambda bits) = r0*h0 + r1*h1 + x1 */
void galas_zk_hash_final(uint8_t* h, galas_zk_hash_ctx* ctx, const gf_limb_t* x1);

/* leaf hash: h (2*lambda bits) = u * x0 + x1, where u is 2*lambda bits (the
   universal-hash key from H0) and x0,x1 are lambda bits. Uses the UNREDUCED
   product (extension ring), matching FAEST leaf_hash. */
void galas_leaf_hash(uint8_t* h /* 2*lambda bits */, const uint8_t* u /* 2*lambda */,
                     const uint8_t* x0 /* lambda */, const uint8_t* x1 /* lambda */,
                     unsigned lambda);

#endif /* GALAS_UNIVERSAL_HASHING_H */
