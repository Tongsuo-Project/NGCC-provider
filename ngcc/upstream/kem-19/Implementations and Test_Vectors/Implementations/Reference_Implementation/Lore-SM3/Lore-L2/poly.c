#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdatomic.h>
#include "poly.h"
#include "ntt.h"
#include "randombytes.h"
#include "reduce.h"
#include "sampler.h"
#include "symmetric.h"
#include "toomcook.h"
#include "bch_codec.h"

/* 32-bit branchless centered reduction optimized for Q=257. Returns: the centered reduced 16-bit integer */
static inline int16_t center_mod_257(int32_t x) {
    // Fast reduction using Fermat prime property.
    int32_t t = (x & 255) - (x >> 8);
    t = (t & 255) - (t >> 8);

    // Map to [-128, 128]
    t += (t >> 31) & 257;

    t -= (((128 - t) >> 31) & 257);

    return (int16_t)t;
}

#if LORE_LEVEL > 1
static struct bch_control *lore_bch_ctx = NULL;
static atomic_flag lore_bch_lock = ATOMIC_FLAG_INIT;
/* Locks the shared codec and initializes it on first use. */
static void acquire_bch(void) {
    /* The BCH library uses mutable scratch buffers in this shared context. */
    while (atomic_flag_test_and_set_explicit(&lore_bch_lock, memory_order_acquire)) {
    }
    if (lore_bch_ctx == NULL) {
        lore_bch_ctx = init_bch(LORE_BCH_M, LORE_BCH_T, 0);
        if (lore_bch_ctx == NULL ||
            lore_bch_ctx->ecc_bytes > LORE_ECC_BYTES ||
            8 * LORE_MSG_BYTES + lore_bch_ctx->ecc_bits > LORE_N)
            abort();
    }
}

static void release_bch(void) {
    atomic_flag_clear_explicit(&lore_bch_lock, memory_order_release);
}
#endif
/* Converts a sparse polynomial to a dense polynomial. */
void poly_from_sparse(poly *r, const poly_sparse *s) {
    memset(r->coeffs, 0, LORE_N * sizeof(int16_t));
    for (int i = 0; i < s->n_coeffs; ++i) {
        r->coeffs[s->pos[i]] = s->val[i];
    }
}

/* Adds two polynomials in the ring Z_t[X]/(X^N+1). */
void poly_add_modt(poly *r, const poly *a, const poly *b) {
    // Reduce each lane before addition can carry into its neighbour. Casting
    // to uint16_t also gives the correct residue for negative coefficients.
    for (int j = 0; j < LORE_N; ++j) {
        uint16_t residue = (uint16_t)(((uint16_t)a->coeffs[j] +
                                       (uint16_t)b->coeffs[j]) & (LORE_T - 1));
        int16_t centered = (int16_t)residue;
#if LORE_T == 4
        centered -= (int16_t)(((residue & 1u) & (residue >> 1)) << 2);
#endif
        r->coeffs[j] = centered;
    }
}

/* Negacyclic dense-by-sparse multiplication modulo t. */
void poly_sparse_mul_modt(poly *r, const poly *a_dense, const poly_sparse *b_sparse) {
    int32_t c_tmp[2 * LORE_N] = {0};

    for (int i = 0; i < b_sparse->n_coeffs; i++) {
        uint16_t degree = b_sparse->pos[i];
        int16_t  value  = b_sparse->val[i];

        if (degree >= LORE_N) continue;

        for (int j = 0; j < LORE_N; j++) {
            c_tmp[degree + j] += (int32_t)a_dense->coeffs[j] * value;
        }
    }

    for (int j = 0; j < LORE_N; j++) {
        int32_t final_val = c_tmp[j] - c_tmp[LORE_N + j];

        int16_t val = (int16_t)(final_val & (LORE_T - 1));

        if (LORE_T == 4) {
            val -= ((val >> 1) & (val & 1)) << 2;
        }

        r->coeffs[j] = val;
    }
}

/* Computes the modular inverse using the Extended Euclidean Algorithm. Returns: the modular inverse of a modulo n, or -1 if not invertible */
static int16_t inv(int16_t a, int16_t n) {
    int16_t t = 0, newt = 1;
    int16_t r = n, newr = a;
    while (newr != 0) {
        int16_t quotient = r / newr;
        int16_t temp_t = t;
        t = newt;
        newt = (int16_t)(temp_t - quotient * newt);
        int16_t temp_r = r;
        r = newr;
        newr = (int16_t)(temp_r - quotient * newr);
    }
    if (r > 1) return -1; // not invertible
    if (t < 0) t = t + n;
    return t;
}

/* Combine a polynomial in CRT form back to Z_tq. */
void poly_crt_combine(poly *r, const poly_crt *pc) {
    const int16_t q_inv_t = inv(LORE_Q, LORE_T);

    for (int i = 0; i < LORE_N; ++i) {

        int16_t a_q = pc->q_poly.coeffs[i];
        int16_t a_t = pc->t_poly.coeffs[i];

        int32_t a_q_std = a_q;
        a_q_std += (a_q_std >> 31) & LORE_Q;

        int32_t a_t_std = a_t;
        a_t_std += (a_t_std >> 31) & LORE_T;

        /* Apply Garner's CRT algorithm to reconstruct the coefficient */

        int32_t h = (a_t_std - a_q_std) & (LORE_T - 1);
        h = (h * q_inv_t) & (LORE_T - 1);

        /*  Reconstruct the final result*/
        int32_t result = a_q_std + (int32_t)LORE_Q * h;
        r->coeffs[i] = (int16_t)result;
    }
}
/* Decompose a polynomial from Z_tq to centered CRT form. */
void poly_crt_decompose(poly_crt *pc, const poly *p) {
    for (int i = 0; i < LORE_N; ++i) {
        pc->q_poly.coeffs[i] = barrett_reduce(p->coeffs[i]);

        int16_t t_coeff = (int16_t)(p->coeffs[i] & (LORE_T - 1));
        if (LORE_T == 4) {
            t_coeff -= ((t_coeff >> 1) & (t_coeff & 1)) << 2;
        }
        pc->t_poly.coeffs[i] = t_coeff;
    }
}

/* Adds two CRT polynomials. */
void poly_crt_add(poly_crt *r, const poly_crt *a, const poly_crt *b) {
    for (int i = 0; i < LORE_N; ++i) {
        r->q_poly.coeffs[i] = center_mod_257(a->q_poly.coeffs[i] + b->q_poly.coeffs[i]);

        int16_t t_coeff = (int16_t)((a->t_poly.coeffs[i] + b->t_poly.coeffs[i]) & (LORE_T - 1));
        if (LORE_T == 4) {
            t_coeff -= ((t_coeff >> 1) & (t_coeff & 1)) << 2;
        }
        r->t_poly.coeffs[i] = t_coeff;
    }
}

/* Subtracts two CRT polynomials. */
void poly_crt_sub(poly_crt *r, const poly_crt *a, const poly_crt *b) {
    for (int i = 0; i < LORE_N; ++i) {
        r->q_poly.coeffs[i] = barrett_reduce(a->q_poly.coeffs[i] - b->q_poly.coeffs[i]);

        int16_t t_coeff = (int16_t)((a->t_poly.coeffs[i] - b->t_poly.coeffs[i]) & (LORE_T - 1));
        if (LORE_T == 4) {
            t_coeff -= ((t_coeff >> 1) & (t_coeff & 1)) << 2;
        }
        r->t_poly.coeffs[i] = t_coeff;
    }
}

/* Adds two polynomials. */
 void poly_add(poly *r, const poly *a, const poly *b) {
     for(int i=0; i<LORE_N; ++i) {
         r->coeffs[i] = a->coeffs[i] + b->coeffs[i];
     }
     for(int i=0; i<LORE_N; ++i) {
         r->coeffs[i] = barrett_reduce(r->coeffs[i]);
     }
 }

/* Deserializes a polynomial from a byte array. */
void poly_frombytes(poly *r, const unsigned char *a) {
    for (int i = 0; i < LORE_N / 2; i++) {
        r->coeffs[2 * i] = ((a[3 * i + 0] >> 0) | ((int16_t)a[3 * i + 1] << 8)) & 0xFFF;
        r->coeffs[2 * i + 1] = ((a[3 * i + 1] >> 4) | ((int16_t)a[3 * i + 2] << 4)) & 0xFFF;
    }
}

/* Serializes a polynomial into a byte array. */
void poly_tobytes(unsigned char *r, const poly *p) {
    for (int i = 0; i < LORE_N / 2; i++) {
        r[3 * i + 0] = (unsigned char)(p->coeffs[2 * i + 0] >> 0);
        r[3 * i + 1] = (unsigned char)(((p->coeffs[2 * i + 0] >> 8) & 0xFF) | (p->coeffs[2 * i + 1] << 4));
        r[3 * i + 2] = (unsigned char)(p->coeffs[2 * i + 1] >> 4);
    }
}

/* Samples a noise polynomial with a fixed weight. */
void poly_getnoise(poly_crt_vec *r_crt_vec, poly_sparse *r_sparse_vec, const unsigned char *seed, unsigned char nonce)
{
  sample_fixed_weight(r_crt_vec, r_sparse_vec, seed, nonce);
}

/* Uniform sampling modulo t. */
void poly_getnoise_uniform(poly *r, uint16_t t, const unsigned char *seed, unsigned char nonce)
{
    uint8_t buf[LORE_N * 2];
    prf(buf, sizeof(buf), seed, nonce);

    int ctr = 0;
    unsigned int buf_pos = 0;

    // Direct sampling in [-(t-1)/2, (t-1)/2].
    uint16_t range = t;
    uint16_t limit = (uint16_t)((0x100 / range) * range);

    while(ctr < LORE_N) {
        if (buf_pos >= sizeof(buf)) break;
        uint8_t val = buf[buf_pos++];

        if (val < limit) {
            int16_t t_coeff = (int16_t)(val % range);
            if (t == 4) {
                t_coeff -= ((t_coeff >> 1) & (t_coeff & 1)) << 2;
            }
            r->coeffs[ctr++] = t_coeff;
        }
    }
}
/* Applies Number Theoretic Transform (NTT) to a polynomial. */
void poly_ntt(poly *r) {
    ntt(r->coeffs);
    poly_reduce(r);
}

/* Applies inverse NTT to a polynomial. */
void poly_invntt_tomont(poly *r) {
    invntt_tomont(r->coeffs);
}

/* Pointwise multiplication of two polynomials in the NTT domain. */
void poly_pointwise_montgomery(poly *r, const poly *a, const poly *b) {
    poly_mul_ntt(r->coeffs, a->coeffs, b->coeffs);
}

/* Packs the t-part indices into a byte array. */
#if LORE_R_BITS > 0
void pack_t_bits(unsigned char *r, const uint8_t *t_indices) {
    memset(r, 0, LORE_POLY_COMPRESSED_BYTES_T);
    for (size_t i = 0; i < LORE_N; i++) {
        for (size_t j = 0; j < LORE_R_BITS; j++) {
            if (((t_indices[i] >> j) & 1)) {
                r[(i * LORE_R_BITS + j) / 8] |= (unsigned char)(1 << ((i * LORE_R_BITS + j) % 8));
            }
        }
    }
}

/* Unpacks the t-part indices from a byte array. */
void unpack_t_bits(uint8_t *t_indices, const unsigned char *r) {
    memset(t_indices, 0, LORE_N);
    for (size_t i = 0; i < LORE_N; i++) {
        for (size_t j = 0; j < LORE_R_BITS; j++) {
            if (((r[(i * LORE_R_BITS + j) / 8] >> ((i * LORE_R_BITS + j) % 8)) & 1)) {
               t_indices[i] |= (uint8_t)(1 << j);
            }
        }
    }
}
#endif

/* At t=4, ties round positively to representatives {0,2}. Return the index. */
int16_t find_closest_r_idx(int16_t x_t) {
    #if LORE_T == 4
        unsigned residue = (unsigned)x_t & 3u;
        return (residue == 1u) | (residue == 2u);
    #else
        (void)x_t;
        return 0;
    #endif
}

/* Decodes a message from a CRT polynomial. */
void poly_decode_msg_crt(unsigned char *msg, const poly_crt *r_crt) {
    const int32_t TQ = LORE_T * LORE_Q;
    const int32_t TQ_HALF = TQ / 2;
    memset(msg, 0, LORE_MSG_BYTES);

#if LORE_LEVEL > 1
    acquire_bch();
    unsigned char recv_codeword[LORE_CODEWORD_BYTES] = {0};
    const int decode_bytes = LORE_MSG_BYTES + (lore_bch_ctx->ecc_bits + 7) / 8;
#endif

// ================= Level 1: Repetition Code =================
#if LORE_LEVEL == 1
    for (int i = 0; i < LORE_MSG_BYTES; ++i) {
        for (int j = 0; j < 8; ++j) {
            int32_t dist_to_0 = 0;
            int32_t dist_to_1 = 0;

            for (int k = 0; k < LORE_K_VEC; k++) {
                int idx = (i * 8 + j) * LORE_K_VEC + k;
                int32_t a_q = r_crt->q_poly.coeffs[idx];
                int32_t a_t = r_crt->t_poly.coeffs[idx];

                /* Constant time reductions to [-Q/2, Q/2] and [-T/2, T/2] */
                a_q = a_q % LORE_Q;
                a_q += (a_q >> 31) & LORE_Q;
                a_q -= ((LORE_Q / 2 - a_q) >> 31) & LORE_Q;

                a_t = a_t % LORE_T;
                a_t += (a_t >> 31) & LORE_T;
                a_t -= ((LORE_T / 2 - a_t) >> 31) & LORE_T;

                int32_t h = (a_t - a_q) % LORE_T;
                h += (h >> 31) & LORE_T;
                h -= ((LORE_T / 2 - h) >> 31) & LORE_T;
                // CRT expansion.
                int32_t val = a_q + LORE_Q * h;
                val %= TQ;
                if (val < 0) val += TQ;

                int32_t d0 = val;
                if (d0 > TQ_HALF) d0 = TQ - d0;
                int32_t d1 = abs(val - TQ_HALF);

                dist_to_0 += d0;
                dist_to_1 += d1;
            }

            int32_t diff = dist_to_1 - dist_to_0;
            uint32_t mask = (uint32_t)(diff >> 31);
            msg[i] |= (unsigned char)( (mask & 1) << j );
        }
    }

// ================= Level 2, 3, 4: BCH =================
#else
    for (int i = 0; i < decode_bytes * 8; ++i) {
        int32_t a_q = r_crt->q_poly.coeffs[i];
        int32_t a_t = r_crt->t_poly.coeffs[i];

        a_q = a_q % LORE_Q;
        a_q += (a_q >> 31) & LORE_Q;
        a_q -= ((LORE_Q / 2 - a_q) >> 31) & LORE_Q;

        a_t = a_t % LORE_T;
        a_t += (a_t >> 31) & LORE_T;
        a_t -= ((LORE_T / 2 - a_t) >> 31) & LORE_T;

        int32_t h = (a_t - a_q) % LORE_T;
        h += (h >> 31) & LORE_T;
        h -= ((LORE_T / 2 - h) >> 31) & LORE_T;

        // CRT expansion.
        int32_t val = a_q + LORE_Q * h;
        val %= TQ;
        if (val < 0) val += TQ;

        int32_t d0 = val;
        if (d0 > TQ_HALF) d0 = TQ - d0;
        int32_t d1 = abs(val - TQ_HALF);

        int32_t diff = d1 - d0;
        uint32_t mask = (uint32_t)diff >> 31;
        recv_codeword[i / 8] |= (unsigned char)(mask << (i % 8));
    }

    unsigned char *recv_data = recv_codeword;
    unsigned char *recv_ecc = recv_codeword + LORE_MSG_BYTES;
    unsigned int errloc[LORE_BCH_T];

    int num_err = decode_bch(lore_bch_ctx, recv_data, LORE_MSG_BYTES, recv_ecc, NULL, NULL, errloc);
    if (num_err > 0) {
        correct_bch(lore_bch_ctx, recv_data, LORE_MSG_BYTES, errloc, num_err);
    }
    if (num_err >= 0) {
        memcpy(msg, recv_data, LORE_MSG_BYTES);
    } else {
        memset(msg, 0, LORE_MSG_BYTES);
    }
    release_bch();
#endif
}
/* Encodes a message into a polynomial. */
void poly_encode_msg(poly *r, const unsigned char *msg) {
    const int32_t val = (LORE_T * LORE_Q ) / 2;
    memset(r->coeffs, 0, sizeof(int16_t) * LORE_N);

#if LORE_LEVEL == 1
    for (int i = 0; i < LORE_MSG_BYTES; ++i) {
        for (int j = 0; j < 8; ++j) {
            int bit = (msg[i] & (1 << j)) ? 1 : 0;
            for (int k = 0; k < LORE_K_VEC; k++) {
                r->coeffs[(i * 8 + j) * LORE_K_VEC + k] = (int16_t)(val * bit);
            }
        }
    }
#else
    acquire_bch();
    unsigned char ecc[LORE_ECC_BYTES] = {0};
    unsigned char codeword[LORE_CODEWORD_BYTES] = {0};

    encode_bch(lore_bch_ctx, msg, LORE_MSG_BYTES, ecc);
    memcpy(codeword, msg, LORE_MSG_BYTES);
    memcpy(codeword + LORE_MSG_BYTES, ecc, LORE_ECC_BYTES);

    const int encode_bytes = LORE_MSG_BYTES + (lore_bch_ctx->ecc_bits + 7) / 8;
    release_bch();
    for (int i = 0; i < encode_bytes; ++i) {
        for (int j = 0; j < 8; ++j) {
            r->coeffs[8 * i + j] = (int16_t)(val * ((codeword[i] >> j) & 1u));
        }
    }
#endif

}
/* Decodes a message from a polynomial. */
void poly_decode_msg(unsigned char *msg, const poly *r) {
    const int32_t TQ = LORE_T * LORE_Q;

    memset(msg, 0, LORE_MSG_BYTES);

#if LORE_LEVEL == 1
    for (int i = 0; i < LORE_MSG_BYTES; ++i) {
        for (int j = 0; j < 8; ++j) {
            int32_t S = 0;
            for (int k = 0; k < LORE_K_VEC; k++) {
                int32_t val = r->coeffs[(i * 8 + j) * LORE_K_VEC + k];
                if (val > TQ / 2) val -= TQ;
                S += val;
            }
            if (abs(S) >= TQ / 2) {
                msg[i] |= (unsigned char)(1 << j);
            }
        }
    }
#else
    const int32_t threshold = TQ / 4;
    acquire_bch();
    unsigned char recv_codeword[LORE_CODEWORD_BYTES] = {0};

    const int decode_bytes = LORE_MSG_BYTES + (lore_bch_ctx->ecc_bits + 7) / 8;
    for (int i = 0; i < decode_bytes; ++i) {
        for (int j = 0; j < 8; ++j) {
            int32_t val = r->coeffs[i * 8 + j];
            if (val > TQ / 2) val -= TQ;
            if (abs(val) > threshold) {
                recv_codeword[i] |= (unsigned char)(1 << j);
            }
        }
    }

    unsigned char *recv_data = recv_codeword;
    unsigned char *recv_ecc = recv_codeword + LORE_MSG_BYTES;
    unsigned int errloc[LORE_BCH_T];

    int num_err = decode_bch(lore_bch_ctx, recv_data, LORE_MSG_BYTES, recv_ecc, NULL, NULL, errloc);
    if (num_err > 0) {
        correct_bch(lore_bch_ctx, recv_data, LORE_MSG_BYTES, errloc, num_err);
    }

    if (num_err >= 0) {
        memcpy(msg, recv_data, LORE_MSG_BYTES);
    } else {
        memset(msg, 0, LORE_MSG_BYTES);
    }
    release_bch();
#endif
}
/* Adds an encoded message to a CRT polynomial. */
void poly_add_scaled_msg(poly_crt *r, const poly *msg_poly) {
    for(int i = 0; i < LORE_N; ++i) {
        int32_t m_val = msg_poly->coeffs[i];

        // --- q-part addition ---
        int32_t q_sum = r->q_poly.coeffs[i] + m_val;
        r->q_poly.coeffs[i] = center_mod_257(q_sum);

        // --- t-part addition ---
        int16_t t_coeff = (int16_t)((r->t_poly.coeffs[i] + m_val) & (LORE_T - 1));
        if (LORE_T == 4) {
            t_coeff -= ((t_coeff >> 1) & (t_coeff & 1)) << 2;
        }
        r->t_poly.coeffs[i] = t_coeff;
    }
}

/* Rejection sampling for q-part coefficients. Returns: number of sampled integers (at most len) */
unsigned int rej_uniform_q(int16_t *r,
                           unsigned int len,
                           const uint8_t *buf,
                           unsigned int buflen)
{
  unsigned int ctr = 0, pos = 0;
  uint16_t val;

  while(ctr < len && pos + 2 <= buflen) {
    val = (uint16_t)(buf[pos] | ((uint16_t)buf[pos+1] << 8));
    pos += 2;

    if (val < 65535) {
#if LORE_LEVEL == 4
      // 256 = -1 (mod 257), so a 16-bit word reduces to low - high.
      int16_t residue = (int16_t)buf[pos - 2] - (int16_t)buf[pos - 1];
      r[ctr++] = residue < 0 ? residue + LORE_Q : residue;
#else
      // Exactly 255 accepted 16-bit inputs for each residue modulo 257.
      // Reducing after a signed int16_t cast changes the residue for the
      // upper half of the 16-bit range.
      r[ctr++] = (int16_t)(val % LORE_Q);
#endif
    }
  }
  return ctr;
}

/* Rejection sampling for t-part coefficients. Returns: number of sampled integers (at most len) */
unsigned int rej_uniform_t(int16_t *r,
                           unsigned int len,
                           const uint8_t *buf,
                           unsigned int buflen)
{
  unsigned int ctr = 0, pos = 0;
  const uint16_t t_limit = (0x100 / LORE_T) * LORE_T;

  while(ctr < len && pos < buflen) {
    if (buf[pos] < t_limit) {
      int16_t t_coeff = (int16_t)(buf[pos] & (LORE_T - 1));
      if (LORE_T == 4) {
          t_coeff -= ((t_coeff >> 1) & (t_coeff & 1)) << 2;
      }
      r[ctr++] = t_coeff;
    }
    pos++;
  }
  return ctr;
}
/* Karatsuba multiplication of two polynomials in Z_t[X]/(X^N+1). */
void poly_mul_modt(poly *r, const poly *a, const poly *b) {
    int16_t res_full[LORE_N];
    memset(res_full, 0, LORE_N * sizeof(int16_t));
    poly_mul_acc(a->coeffs, b->coeffs, res_full); // Underlying polynomial multiplication.

    for (int i = 0; i < LORE_N; i++) {
        int32_t final_val = res_full[i];

        int16_t val = (int16_t)(final_val & (LORE_T - 1));
        if (LORE_T == 4) {
            val -= ((val >> 1) & (val & 1)) << 2;
        }

        r->coeffs[i] = val;
    }
}

/* Splits the q-part of a polynomial into a main buffer and an overflow buffer. Returns: the number of overflow bits generated */
size_t lore_poly_pack_q_split(unsigned char main_buf[LORE_N], unsigned char *overflow_buf, const poly *p) {
    int total_overflow_bits = 0;

    for (int j = 0; j < LORE_N; ++j) {
        int16_t coeff = p->coeffs[j];
        /* Constant time reduction */
        coeff = barrett_reduce(coeff);
        coeff += (coeff >> 15) & LORE_Q;

        if (coeff < 255) {
            main_buf[j] = (unsigned char)coeff;
        } else {
            main_buf[j] = 0xFF; // Mark as overflow

            // Ensure the target byte is clean before writing the first overflow bit
            if ((total_overflow_bits % 8) == 0) {
                overflow_buf[total_overflow_bits / 8] = 0;
            }

            if (coeff == 256) {
                // If the coefficient is 256, set the corresponding bit to 1
                overflow_buf[total_overflow_bits / 8] |= (unsigned char)(1 << (total_overflow_bits % 8));
            }
            // If the coefficient is 255, we do nothing; the bit remains 0
            total_overflow_bits++;
        }
    }
    return (size_t)total_overflow_bits;
}

/* Reconstructs a polynomial from a main buffer and an overflow buffer. */
void lore_poly_unpack_q_split(poly *p, const unsigned char main_buf[LORE_N], const unsigned char *overflow_buf, int *overflow_bit_pos) {
    for (int j = 0; j < LORE_N; ++j) {
        unsigned char byte = main_buf[j];
        if (byte == 0xFF) {
            int bit = (overflow_buf[*overflow_bit_pos / 8] >> (*overflow_bit_pos % 8)) & 1;
            (*overflow_bit_pos)++;
            p->coeffs[j] = (int16_t)(255 + bit);
        } else {
            p->coeffs[j] = byte;
        }
    }
}

/* Normal domain schoolbook multiplication mod (Q, X^N+1) */
void poly_mul_schoolbook_q(poly *r, const poly *a, const poly *b) {
    int32_t c_tmp[2 * LORE_N] = {0};

    for (int i = 0; i < LORE_N; i++) {
        for (int j = 0; j < LORE_N; j++) {
            c_tmp[i + j] += (int32_t)a->coeffs[i] * b->coeffs[j];
        }
    }

    for (int j = 0; j < LORE_N; j++) {
        int32_t val = c_tmp[j] - c_tmp[j + LORE_N];

        // Apply 32-bit reduction.
        r->coeffs[j] = center_mod_257(val);
    }
}
