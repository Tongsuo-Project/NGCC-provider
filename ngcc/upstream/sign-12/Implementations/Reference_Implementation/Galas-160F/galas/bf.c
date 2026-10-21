/*
 * bf.c — QuickSilver big-field contexts for GALAS. See bf.h.
 *
 * Each bfN_t is a GF(2^N) element whose arithmetic is delegated to a lazily-
 * initialized gf_ctx. The reduction polynomials match the GALAS field
 * definitions (params/field_defs.md), which for N in {128,256} also match
 * FAEST's bfN_modulus — so a faithful port of universal_hashing.c works.
 *
 * NOTE on GF(2^64): FAEST's bf64 uses modulus x^64+x^4+x^3+x+1 (= 0x1B). This
 * is NOT one of our GALAS lambda fields, but zk_hash needs it for the t-side
 * accumulator. We add it here as a special case.
 */
#include "bf.h"
#include <string.h>

/* GF(2^64) with FAEST's bf64 modulus x^64+x^4+x^3+x+1 (low bits 0x1B). The
   gf_init() table doesn't have 64, so we build the ctx manually. */
static gf_ctx ctx64;
static int ctx64_ready = 0;
static const gf_ctx* bf_ctx_64_impl(void) {
    if (!ctx64_ready) {
        memset(&ctx64, 0, sizeof(ctx64));
        ctx64.n = 64; ctx64.nbits = 64; ctx64.nbytes = 8; ctx64.nlimbs = 1;
        ctx64.phi[0] = 0x1B;          /* x^4+x^3+x+1 */
        ctx64.phi[1] = 1;             /* x^64 */
        ctx64_ready = 1;
    }
    return &ctx64;
}
const gf_ctx* bf_ctx_64(void)  { return bf_ctx_64_impl(); }

#define LAZY(N) \
    static gf_ctx ctx##N; static int ready##N = 0; \
    const gf_ctx* bf_ctx_##N(void) { \
        if (!ready##N) { gf_init(&ctx##N, N); ready##N = 1; } \
        return &ctx##N; \
    }
LAZY(160)
LAZY(256)
LAZY(384)
LAZY(512)

/* The 2*lambda extension fields (bf320, bf768, bf1024) are used by FAEST for
   leaf_hash products. Their unreduced product (bf_mul_unreduced) needs no
   modulus, so bf_ctx_320/768/1024 are not needed for the unreduced path. If a
   future reduction over these extension fields is required, their irreducible
   polynomials must be selected and verified first. Left as a stub returning
   NULL to force an explicit error if anything tries to reduce over them. */
const gf_ctx* bf_ctx_320(void)  { return NULL; }
const gf_ctx* bf_ctx_768(void)  { return NULL; }
const gf_ctx* bf_ctx_1024(void) { return NULL; }

/* Unreduced product: out (2*nbits) <- a*b as a polynomial. Schoolbook over the
   bits of a (shift b into the accumulator). Correctness favored over speed. */
void bf_mul_unreduced(gf_limb_t* out, const gf_limb_t* a, const gf_limb_t* b,
                      unsigned nbits) {
    unsigned la = (nbits + 63) / 64;
    unsigned lout = (2 * nbits + 63) / 64;
    for (unsigned i = 0; i < lout; ++i) out[i] = 0;
    /* acc <- b; for each bit k of a (low->high): if set, XOR acc into out;
       then shift acc left by 1 (no reduction). */
    gf_limb_t acc[GF_LIMBS(1024)];
    for (unsigned i = 0; i < la; ++i) acc[i] = b[i];
    for (unsigned i = la; i < lout; ++i) acc[i] = 0;
    for (unsigned k = 0; k < nbits; ++k) {
        if ((a[k / 64] >> (k % 64)) & 1) {
            for (unsigned i = 0; i < lout; ++i) out[i] ^= acc[i];
        }
        /* acc <<= 1 across lout limbs */
        uint64_t carry = 0;
        for (unsigned i = 0; i < lout; ++i) {
            uint64_t next = (acc[i] >> 63) & 1;
            acc[i] = (acc[i] << 1) | carry;
            carry = next;
        }
    }
}
