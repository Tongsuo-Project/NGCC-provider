/*
 * xof.h — GALAS XOF/hash abstraction layer (v2: hash vs PRG separation).
 *
 * NGCC provides three primitives with DIFFERENT security strengths:
 *   sm3hash(256)     → 128-bit security
 *   pseudohash(N)    → N/2-bit security (N ∈ {512, 768, 1024})
 *   pseudoXOF        → 128-bit security (SM3 counter mode, fixed)
 *
 * VOLEitH requires hash security matching the security parameter. For
 * lambda > 128, pseudoXOF alone is insufficient.
 * Therefore:
 *   - HASH operations (H0-H4, challenges, commitments): use the supported
 *     pseudohash variant selected by xof_pseudohash_size(lambda). For outputs
 *     longer than one digest, call pseudohash with a counter and concatenate.
 *   - PRG operations (GGM tree, ConvertToVole): use pseudoXOF
 *     (security guaranteed by seed length, not collision resistance).
 *
 * This layer provides a unified absorb/squeeze API that internally routes
 * to the correct NGCC function based on the domain label.  The label is not
 * absorbed automatically; callers such as random_oracle.c append the FAEST
 * separator byte explicitly before finalization.
 */
#ifndef GALAS_XOF_H
#define GALAS_XOF_H

#include <stdint.h>
#include <stddef.h>

/* Backend-selection labels. Labels >= 0x20 are PRG (pseudoXOF);
   labels < 0x20 are hash (pseudohash with size from xof_pseudohash_size). */
typedef enum {
    XOF_DOMAIN_H0 = 0x10,    /* leaf-hash key derivation → pseudohash */
    XOF_DOMAIN_H1 = 0x11,    /* commitment hash → pseudohash */
    XOF_DOMAIN_H2 = 0x12,    /* Fiat-Shamir challenge → pseudohash */
    XOF_DOMAIN_H3 = 0x13,    /* seed derivation → pseudohash */
    XOF_DOMAIN_H4 = 0x14,    /* IV derivation → pseudohash */
    XOF_DOMAIN_TREE_PRG = 0x20,   /* GGM tree expansion → pseudoXOF */
    XOF_DOMAIN_LEAF   = 0x21,     /* leaf commitment → pseudoXOF (PRG from seed) */
    XOF_DOMAIN_KDF    = 0x30,     /* general PRG derivation → pseudoXOF */
} xof_domain_t;

/* Check if a domain is a hash domain (→ pseudohash) or PRG (→ pseudoXOF) */
#define XOF_IS_HASH_DOMAIN(d) ((d) < 0x20)

typedef struct {
    unsigned lambda_bits;
    uint8_t  domain;
    uint8_t* buf;
    size_t   buf_len;
    size_t   buf_cap;
    int      finalized;
    uint8_t* out;
    size_t   out_cap;
    size_t   out_have;
    size_t   out_used;
} xof_ctx;

void xof_init(xof_ctx* ctx, unsigned lambda_bits, xof_domain_t domain);
void xof_update(xof_ctx* ctx, const void* data, size_t len);
void xof_update_u16(xof_ctx* ctx, uint16_t v);
void xof_update_u32(xof_ctx* ctx, uint32_t v);
void xof_final(xof_ctx* ctx);
void xof_squeeze(xof_ctx* ctx, void* out, size_t len);
void xof_clear(xof_ctx* ctx);

void xof_oneshot(unsigned lambda_bits, xof_domain_t domain,
                 const void* data, size_t len, void* out, size_t out_len);

/* Helper: determine the pseudohash digest size for a given lambda.
   Returns 512/768/1024 (the smallest supported digest with output >= 2*lambda). */
static inline int xof_pseudohash_size(unsigned lambda) {
    if (lambda <= 256) return 512;
    if (lambda <= 384) return 768;
    return 1024;
}

#endif /* GALAS_XOF_H */
