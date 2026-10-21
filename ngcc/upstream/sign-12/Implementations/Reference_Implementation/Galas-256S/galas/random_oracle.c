/*
 * random_oracle.c — GALAS Fiat-Shamir oracles over xof_* (NGCC pseudoXOF).
 * Port of ref-FAEST random_oracle.c; domain separators preserved.
 */
#include "random_oracle.h"
#include <string.h>

static const uint8_t DS_H0   = 0;
static const uint8_t DS_H1   = 1;
static const uint8_t DS_H2_0 = 8 + 0;
static const uint8_t DS_H2_1 = 8 + 1;
static const uint8_t DS_H2_2 = 8 + 2;
static const uint8_t DS_H2_3 = 8 + 3;
static const uint8_t DS_H3   = 3;
static const uint8_t DS_H4   = 4;

/* H0 */
void galas_H0_init(galas_H0_ctx* ctx, unsigned lambda) {
    /* FAEST picks shake128 for lambda==128 else shake256. Our pseudoXOF is
       domain-separated, so we use a single domain (H0) regardless of lambda. */
    xof_init(ctx, lambda, XOF_DOMAIN_H0);
}
void galas_H0_update(galas_H0_ctx* ctx, const uint8_t* src, size_t len) { xof_update(ctx, src, len); }
void galas_H0_final_for_squeeze(galas_H0_ctx* ctx) {
    xof_update(ctx, &DS_H0, sizeof(DS_H0));
    xof_final(ctx);
}
void galas_H0_squeeze(galas_H0_ctx* ctx, uint8_t* dst, size_t len) { xof_squeeze(ctx, dst, len); }
void galas_H0_clear(galas_H0_ctx* ctx) { xof_clear(ctx); }

/* H1 */
void galas_H1_init(galas_H1_ctx* ctx, unsigned lambda) { xof_init(ctx, lambda, XOF_DOMAIN_H1); }
void galas_H1_update(galas_H1_ctx* ctx, const uint8_t* src, size_t len) { xof_update(ctx, src, len); }
void galas_H1_final(galas_H1_ctx* ctx, uint8_t* digest, size_t len) {
    xof_update(ctx, &DS_H1, sizeof(DS_H1));
    xof_final(ctx);
    xof_squeeze(ctx, digest, len);
    xof_clear(ctx);
}

/* H2 (challenges) */
void galas_H2_init(galas_H2_ctx* ctx, unsigned lambda) { xof_init(ctx, lambda, XOF_DOMAIN_H2); }
void galas_H2_update(galas_H2_ctx* ctx, const uint8_t* src, size_t len) { xof_update(ctx, src, len); }
static void h2_final(galas_H2_ctx* ctx, uint8_t ds, uint8_t* digest, size_t len) {
    xof_update(ctx, &ds, sizeof(ds));
    xof_final(ctx);
    xof_squeeze(ctx, digest, len);
    xof_clear(ctx);
}
void galas_H2_0_final(galas_H2_ctx* ctx, uint8_t* d, size_t n) { h2_final(ctx, DS_H2_0, d, n); }
void galas_H2_1_final(galas_H2_ctx* ctx, uint8_t* d, size_t n) { h2_final(ctx, DS_H2_1, d, n); }
void galas_H2_2_final(galas_H2_ctx* ctx, uint8_t* d, size_t n) { h2_final(ctx, DS_H2_2, d, n); }
void galas_H2_3_final(galas_H2_ctx* ctx, uint8_t* d, size_t n) { h2_final(ctx, DS_H2_3, d, n); }

/* H3 */
void galas_H3_init(galas_H3_ctx* ctx, unsigned lambda) { xof_init(ctx, lambda, XOF_DOMAIN_H3); }
void galas_H3_update(galas_H3_ctx* ctx, const uint8_t* src, size_t len) { xof_update(ctx, src, len); }
void galas_H3_final(galas_H3_ctx* ctx, uint8_t* digest, size_t dlen, uint8_t* iv) {
    xof_update(ctx, &DS_H3, sizeof(DS_H3));
    xof_final(ctx);
    xof_squeeze(ctx, digest, dlen);
    xof_squeeze(ctx, iv, GALAS_IV_SIZE);
    xof_clear(ctx);
}

/* H4 */
void galas_H4_init(galas_H4_ctx* ctx, unsigned lambda) { xof_init(ctx, lambda, XOF_DOMAIN_H4); }
void galas_H4_update(galas_H4_ctx* ctx, const uint8_t* iv) { xof_update(ctx, iv, GALAS_IV_SIZE); }
void galas_H4_final(galas_H4_ctx* ctx, uint8_t* iv) {
    xof_update(ctx, &DS_H4, sizeof(DS_H4));
    xof_final(ctx);
    xof_squeeze(ctx, iv, GALAS_IV_SIZE);
    xof_clear(ctx);
}
