/*
 * gf2n.c — portable GF(2^n) arithmetic. See gf2n.h.
 *
 * All routines work on limb arrays of length ctx->nlimbs. Multiplication uses
 * the schoolbook shift-XOR-reduce algorithm (slow but obviously correct); for
 * the reference implementation this is acceptable. n=512 mul is the hottest
 * path but still fine for a reference.
 */
#include "gf2n.h"
#include <string.h>

typedef struct { unsigned nn; uint64_t pp[GF_LIMBS(512) + 2]; } field_rec;

int gf_init(gf_ctx* ctx, unsigned n) {
    const field_rec* rec = 0;
    static const field_rec table[] = {
        {128, {0x0000000000000087ULL, 0}},
        {160, {0x000000000000002dULL, 0, 0}},
        {256, {0x0000000000000425ULL, 0, 0, 0, 0}},
        {384, {0x0000000000018041ULL, 0, 0, 0, 0, 0, 0}},
        {512, {0x0000000000000125ULL, 0, 0, 0, 0, 0, 0, 0, 0}},
    };
    for (size_t i = 0; i < sizeof(table) / sizeof(table[0]); ++i) {
        if (table[i].nn == n) { rec = &table[i]; break; }
    }
    if (!rec) return -1;
    memset(ctx, 0, sizeof(*ctx));
    ctx->n = n;
    ctx->nbits = n;
    ctx->nbytes = (n + 7) / 8;
    ctx->nlimbs = GF_LIMBS(n);
    for (unsigned i = 0; i < ctx->nlimbs + 1; ++i)
        ctx->phi[i] = rec->pp[i];
    unsigned top_limb = n / 64;
    unsigned top_bit = n % 64;
    ctx->phi[top_limb] |= ((gf_limb_t)1 << top_bit);
    return 0;
}

void gf_zero(const gf_ctx* ctx, gf_limb_t* out) {
    for (unsigned i = 0; i < ctx->nlimbs; ++i) out[i] = 0;
}
void gf_copy(const gf_ctx* ctx, gf_limb_t* out, const gf_limb_t* a) {
    for (unsigned i = 0; i < ctx->nlimbs; ++i) out[i] = a[i];
}
int gf_is_zero(const gf_ctx* ctx, const gf_limb_t* a) {
    for (unsigned i = 0; i < ctx->nlimbs; ++i) if (a[i]) return 0;
    return 1;
}
void gf_from_bytes(const gf_ctx* ctx, gf_limb_t* out, const uint8_t* in) {
    gf_zero(ctx, out);
    for (unsigned i = 0; i < ctx->nbytes; ++i)
        out[i / 8] |= ((gf_limb_t)in[i]) << (8 * (i % 8));
}
void gf_to_bytes(const gf_ctx* ctx, uint8_t* out, const gf_limb_t* a) {
    for (unsigned i = 0; i < ctx->nbytes; ++i)
        out[i] = (uint8_t)(a[i / 8] >> (8 * (i % 8)));
}
void gf_add(const gf_ctx* ctx, gf_limb_t* out, const gf_limb_t* a, const gf_limb_t* b) {
    for (unsigned i = 0; i < ctx->nlimbs; ++i) out[i] = a[i] ^ b[i];
}
void gf_xor(const gf_ctx* ctx, gf_limb_t* a, const gf_limb_t* b) {
    for (unsigned i = 0; i < ctx->nlimbs; ++i) a[i] ^= b[i];
}

static inline int wide_bit(const gf_limb_t* a, unsigned bit) {
    return (int)((a[bit / 64] >> (bit % 64)) & 1u);
}

static inline void wide_flip_bit(gf_limb_t* a, unsigned bit) {
    a[bit / 64] ^= ((gf_limb_t)1 << (bit % 64));
}

static unsigned low_terms(const gf_ctx* ctx, unsigned terms[64]) {
    unsigned nterms = 0;
    for (unsigned bit = 0; bit < ctx->n; ++bit) {
        if ((ctx->phi[bit / 64] >> (bit % 64)) & 1u) {
            terms[nterms++] = bit;
        }
    }
    return nterms;
}

static void gf_reduce_wide(const gf_ctx* ctx, gf_limb_t* out, gf_limb_t* wide) {
    unsigned terms[64];
    unsigned nterms = low_terms(ctx, terms);
    unsigned L = ctx->nlimbs;

    for (int bit = (int)(2u * ctx->n - 2u); bit >= (int)ctx->n; --bit) {
        if (!wide_bit(wide, (unsigned)bit)) continue;
        wide_flip_bit(wide, (unsigned)bit);
        unsigned base = (unsigned)bit - ctx->n;
        for (unsigned i = 0; i < nterms; ++i) {
            wide_flip_bit(wide, base + terms[i]);
        }
    }

    for (unsigned i = 0; i < L; ++i) out[i] = wide[i];
    if (ctx->n % 64) {
        out[L - 1] &= (((gf_limb_t)1 << (ctx->n % 64)) - 1u);
    }
}

void gf_mul(const gf_ctx* ctx, gf_limb_t* out, const gf_limb_t* a, const gf_limb_t* b) {
    gf_limb_t wide[GF_LIMBS(1024)];
    unsigned L = ctx->nlimbs;
    unsigned W = GF_LIMBS(2u * 512u);
    for (unsigned i = 0; i < W; ++i) wide[i] = 0;

    for (unsigned bit = 0; bit < ctx->n; ++bit) {
        if (((b[bit / 64] >> (bit % 64)) & 1u) == 0) continue;
        unsigned word_shift = bit / 64;
        unsigned bit_shift = bit % 64;
        for (unsigned i = 0; i < L; ++i) {
            wide[i + word_shift] ^= a[i] << bit_shift;
            if (bit_shift && i + word_shift + 1 < W) {
                wide[i + word_shift + 1] ^= a[i] >> (64u - bit_shift);
            }
        }
    }

    gf_reduce_wide(ctx, out, wide);
}

void gf_sqr(const gf_ctx* ctx, gf_limb_t* out, const gf_limb_t* a) {
    gf_limb_t wide[GF_LIMBS(1024)];
    unsigned W = GF_LIMBS(2u * 512u);
    for (unsigned i = 0; i < W; ++i) wide[i] = 0;

    for (unsigned bit = 0; bit < ctx->n; ++bit) {
        if ((a[bit / 64] >> (bit % 64)) & 1u) {
            wide_flip_bit(wide, 2u * bit);
        }
    }
    gf_reduce_wide(ctx, out, wide);
}

void gf_frobenius(const gf_ctx* ctx, gf_limb_t* out, const gf_limb_t* a, unsigned j) {
    gf_copy(ctx, out, a);
    for (unsigned k = 0; k < j; ++k) {
        gf_limb_t tmp[GF_LIMBS(512)];
        gf_sqr(ctx, tmp, out);
        gf_copy(ctx, out, tmp);
    }
}

void gf_inv(const gf_ctx* ctx, gf_limb_t* out, const gf_limb_t* a) {
    /* a^(2^n - 2). Exponent = (111..110)_2 (n bits). */
    if (gf_is_zero(ctx, a)) { gf_zero(ctx, out); return; }
    gf_limb_t acc[GF_LIMBS(512)];
    gf_limb_t base[GF_LIMBS(512)];
    /* acc = 1 */
    gf_zero(ctx, acc); acc[0] = 1;
    gf_copy(ctx, base, a);
    /* scan exponent bits 0..n-1; bit 0 is 0, bits 1..n-1 are 1 */
    for (unsigned bit = 0; bit < ctx->n; ++bit) {
        if (bit != 0) {
            gf_limb_t t[GF_LIMBS(512)];
            gf_mul(ctx, t, acc, base);
            gf_copy(ctx, acc, t);
        }
        gf_limb_t t[GF_LIMBS(512)];
        gf_sqr(ctx, t, base);
        gf_copy(ctx, base, t);
    }
    gf_copy(ctx, out, acc);
}
