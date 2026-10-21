#include "reduce.h"
#include "params.h"
#include <stdint.h>

/* Montgomery reduction: a * 2^-16 modulo q. */
int16_t montgomery_reduce(int64_t a) {
    int32_t t;
    int16_t v;

    // QINV = -255, Q = 257
    v = (int16_t)(a * (-255));
    t = (int64_t)v * LORE_Q;
    t = (int32_t)(a - t);
    t >>= 16;
    return (int16_t)t;
}

/* Fold twice using 256 = -1 (mod 257); the result need not be canonical. */
int16_t barrett_reduce(int16_t a) {

    int16_t t = a;

    // First reduction: t = (t & 0xFF) - (t >> 8)
    // This corresponds to X = r - k (since 2^8 ≡ -1 mod 257)
    t = (t & 0xFF) - (t >> 8);

     t = (t & 0xFF) - (t >> 8);

    return (int16_t)t;
}

/* Applies Barrett reduction to all coefficients of a polynomial. */
// Optimized version (unrolled 4 times)
void poly_reduce(poly *r) {
    for(int i=0; i<LORE_N; i+=4) {
        r->coeffs[i]   = barrett_reduce(r->coeffs[i]);
        r->coeffs[i+1] = barrett_reduce(r->coeffs[i+1]);
        r->coeffs[i+2] = barrett_reduce(r->coeffs[i+2]);
        r->coeffs[i+3] = barrett_reduce(r->coeffs[i+3]);
    }
}


