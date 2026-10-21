/*
 * random_oracle.h — GALAS Fiat-Shamir random oracles (H0..H4).
 *
 * Direct port of RefCodes/ref-FAEST/faest_128f/random_oracle.{h,c}, with the
 * FAEST hash_* calls mapped to the xof_* layer (NGCC pseudoXOF). The domain
 * separators and the H0..H4 structure are unchanged, so a byte-exact cross-
 * check against FAEST is possible (modulo the XOF instantiation difference).
 *
 *   H0: leaf-hash key derivation (universal-hash keys r0..r3,s,t)
 *   H1: commitment Merkle hash
 *   H2: Fiat-Shamir challenges (mu, chall_1, chall_2, chall_3 via H2_0..H2_3)
 *   H3: seed/IV derivation from sk||mu
 *   H4: IV finalization
 */
#ifndef GALAS_RANDOM_ORACLE_H
#define GALAS_RANDOM_ORACLE_H

#include <stdint.h>
#include <stddef.h>
#include "xof.h"

#define GALAS_IV_SIZE 20

/* All H*_context_t are just xof_ctx (incremental absorb/squeeze). */
typedef xof_ctx galas_H0_ctx;
typedef xof_ctx galas_H1_ctx;
typedef xof_ctx galas_H2_ctx;
typedef xof_ctx galas_H3_ctx;
typedef xof_ctx galas_H4_ctx;

void galas_H0_init(galas_H0_ctx* ctx, unsigned lambda);
void galas_H0_update(galas_H0_ctx* ctx, const uint8_t* src, size_t len);
void galas_H0_final_for_squeeze(galas_H0_ctx* ctx);
void galas_H0_squeeze(galas_H0_ctx* ctx, uint8_t* dst, size_t len);
void galas_H0_clear(galas_H0_ctx* ctx);

void galas_H1_init(galas_H1_ctx* ctx, unsigned lambda);
void galas_H1_update(galas_H1_ctx* ctx, const uint8_t* src, size_t len);
void galas_H1_final(galas_H1_ctx* ctx, uint8_t* digest, size_t len);

void galas_H2_init(galas_H2_ctx* ctx, unsigned lambda);
void galas_H2_update(galas_H2_ctx* ctx, const uint8_t* src, size_t len);
void galas_H2_0_final(galas_H2_ctx* ctx, uint8_t* digest, size_t len);  /* mu */
void galas_H2_1_final(galas_H2_ctx* ctx, uint8_t* digest, size_t len);  /* chall_1 */
void galas_H2_2_final(galas_H2_ctx* ctx, uint8_t* digest, size_t len);  /* chall_2 */
void galas_H2_3_final(galas_H2_ctx* ctx, uint8_t* digest, size_t len);  /* chall_3 / delta */

void galas_H3_init(galas_H3_ctx* ctx, unsigned lambda);
void galas_H3_update(galas_H3_ctx* ctx, const uint8_t* src, size_t len);
void galas_H3_final(galas_H3_ctx* ctx, uint8_t* digest, size_t dlen, uint8_t* iv);

void galas_H4_init(galas_H4_ctx* ctx, unsigned lambda);
void galas_H4_update(galas_H4_ctx* ctx, const uint8_t* iv);
void galas_H4_final(galas_H4_ctx* ctx, uint8_t* iv);

#endif
