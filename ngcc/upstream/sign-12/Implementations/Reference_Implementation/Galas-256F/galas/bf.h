/*
 * bf.h — QuickSilver "big field" types (bfN_t) for GALAS.
 *
 * These are GF(2^N) elements used as the VOLE authentication field and its
 * extensions (e.g. bf384 = GF(2^384) for products of two bf192 elements in
 * FAEST). We implement them on top of the already-verified gf2n arithmetic,
 * so correctness is inherited from gf2n.c's tested mul/inv.
 *
 * Sizes needed by GALAS QuickSilver:
 *   bf64_t   GF(2^64)   (the "small field" for zk_hash's t accumulator)
 *   bf160_t  GF(2^160)  (GALAS lambda=160 auth field)
 *   bf256_t  GF(2^256)
 *   bf384_t  GF(2^384)  (= bf192 product in FAEST; for GALAS a standalone field)
 *   bf512_t  GF(2^512)
 *   bf*_t of size 2*lambda for leaf-hash / mul products (bf320, bf768, ...)
 *
 * To keep it simple and uniform, we represent every bfN_t as a limb array of
 * length ceil(N/64) and route all arithmetic through a per-size gf_ctx.
 */
#ifndef GALAS_BF_H
#define GALAS_BF_H

#include <stdint.h>
#include <stddef.h>
#include "gf2n.h"

/* Limb counts (matching GF_LIMBS). */
#define BF64_LIMBS   GF_LIMBS(64)
#define BF160_LIMBS  GF_LIMBS(160)
#define BF256_LIMBS  GF_LIMBS(256)
#define BF320_LIMBS  GF_LIMBS(320)   /* 2*160, for products */
#define BF384_LIMBS  GF_LIMBS(384)
#define BF512_LIMBS  GF_LIMBS(512)
#define BF768_LIMBS  GF_LIMBS(768)   /* 2*384 */
#define BF1024_LIMBS GF_LIMBS(1024)  /* 2*512 */

#define BF_BYTES(n)  ((n) / 8)

typedef gf_limb_t bf64_t[BF64_LIMBS];
typedef gf_limb_t bf160_t[BF160_LIMBS];
typedef gf_limb_t bf256_t[BF256_LIMBS];
typedef gf_limb_t bf320_t[BF320_LIMBS];
typedef gf_limb_t bf384_t[BF384_LIMBS];
typedef gf_limb_t bf512_t[BF512_LIMBS];
typedef gf_limb_t bf768_t[BF768_LIMBS];
typedef gf_limb_t bf1024_t[BF1024_LIMBS];

/* ---- per-size context accessors (initialized lazily, thread-unsafe) ---- */
const gf_ctx* bf_ctx_64(void);
const gf_ctx* bf_ctx_160(void);
const gf_ctx* bf_ctx_256(void);
const gf_ctx* bf_ctx_320(void);
const gf_ctx* bf_ctx_384(void);
const gf_ctx* bf_ctx_512(void);
const gf_ctx* bf_ctx_768(void);
const gf_ctx* bf_ctx_1024(void);

/* Generic ops on a bfN_t given its gf_ctx. The caller picks the right ctx. */
static inline void bf_zero(const gf_ctx* c, gf_limb_t* a) { gf_zero(c, a); }

static inline void bf_load(const gf_ctx* c, gf_limb_t* out, const uint8_t* src) {
    gf_from_bytes(c, out, src);
}
static inline void bf_store(const gf_ctx* c, uint8_t* dst, const gf_limb_t* src) {
    gf_to_bytes(c, dst, src);
}
static inline void bf_add(const gf_ctx* c, gf_limb_t* out, const gf_limb_t* a, const gf_limb_t* b) {
    gf_add(c, out, a, b);
}
/* mul: out <- a*b mod the field's phi. */
static inline void bf_mul(const gf_ctx* c, gf_limb_t* out, const gf_limb_t* a, const gf_limb_t* b) {
    gf_mul(c, out, a, b);
}

/*
 * Product into a 2N-bit field: bfN * bfN -> bf(2N), WITHOUT reduction.
 * This is what FAEST's bf128_mul producing bf384 etc. actually needs: the
 * "extension" products in leaf_hash / vole_hash are unreduced polynomial
 * products (the hash combines them linearly, so reduction happens at the end).
 *
 * out (2N bits) <- a * b as a polynomial (no modulus), stored as 2*limbs.
 */
void bf_mul_unreduced(gf_limb_t* out /* 2*GF_LIMBS(2N) limbs */,
                      const gf_limb_t* a, const gf_limb_t* b,
                      unsigned nbits);

#endif /* GALAS_BF_H */
