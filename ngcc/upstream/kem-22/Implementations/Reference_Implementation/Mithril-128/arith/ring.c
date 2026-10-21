#include "ring.h"
#include "packing.h"
#include "uniform.h"

static uint16_t round_q_to_p_from_u16(uint16_t x)
{
#if RRLWR_PKE_LOGQ == 13 && RRLWR_PKE_LOGP == 11
  int32_t c = (int16_t)(uint16_t)(x << 3);
  c = (c + 16) >> 5;
  return (uint16_t)(c & 0x7ff);
#else
  x <<= 16 - RRLWR_PKE_LOGQ;
  int32_t c = (int16_t)x;
  c >>= 16 - RRLWR_PKE_LOGQ;
  c += (int32_t)1 << (RRLWR_PKE_LOGQ - (RRLWR_PKE_LOGP + 1));
  c >>= RRLWR_PKE_LOGQ - RRLWR_PKE_LOGP;
  return (uint16_t)(c & (RRLWR_PKE_P - 1));
#endif
}

static void poly_mul_x_plus_2(poly *r, const poly *f)
{
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    uint16_t xterm = (i == 0) ? (uint16_t)(-f->coeffs[RRLWR_N - 1]) : f->coeffs[i - 1];
    r->coeffs[i] = (uint16_t)(2 * f->coeffs[i] + xterm);
  }
}

void ring_to_Awin(ring_element_Awin *aw, const ring_element *a)
{
  for(unsigned int u = 0; u < RRLWR_K; u++) {
    aw->x[RRLWR_K - 1 - u] = a->x[u];
  }

  for(unsigned int u = 1; u < RRLWR_K; u++) {
    unsigned int base = RRLWR_K - 1 - u;
    unsigned int dst = 2 * RRLWR_K - 1 - u;

    poly_mul_x_plus_2(&aw->x[dst], &aw->x[base]);
  }
}

void ring_unpack_Awin(ring_element_Awin *aw, const unsigned char *b, int32_t bitlen)
{
  unsigned int offset = bitlen * (RRLWR_N >> 3);

  for(unsigned int u = 0; u < RRLWR_K; u++) {
    poly_unpack(&aw->x[RRLWR_K - 1 - u], b + u * offset, bitlen);
  }

  for(unsigned int u = 1; u < RRLWR_K; u++) {
    unsigned int base = RRLWR_K - 1 - u;
    unsigned int dst = 2 * RRLWR_K - 1 - u;

    poly_mul_x_plus_2(&aw->x[dst], &aw->x[base]);
  }
}

void ring_uniform_Awin(ring_element_Awin *aw,
                       int32_t bitlen,
                       const unsigned char *seed,
                       int32_t seed_len)
{
  ring_uniform_Awin_base(aw, bitlen, seed, seed_len);

  for(unsigned int u = 1; u < RRLWR_K; u++) {
    unsigned int base = RRLWR_K - 1 - u;
    unsigned int dst = 2 * RRLWR_K - 1 - u;

    poly_mul_x_plus_2(&aw->x[dst], &aw->x[base]);
  }
}

/// @brief Ring multiplication over R_q using precomputed A-window rows.
void ring_mul_Awin(poly *r,
                   const ring_element_Awin *a,
                   const ring_element *b,
                   int ncoeffs)
{
  int row_min = RRLWR_K - ncoeffs;
  uint16_t acc[RRLWR_N];

  for(int i = RRLWR_K - 1; i >= row_min; i--) {
    int out = i - row_min;
    const poly *row = &a->x[RRLWR_K - 1 - i];

    poly_mul_toom4_u16(acc, &row[0], &b->x[0]);
    for(int j = 1; j < RRLWR_K; j++) {
      poly_macc_toom4_u16(acc, &row[j], &b->x[j]);
    }

    for(unsigned int k = 0; k < RRLWR_N; k++) {
      r[out].coeffs[k] = acc[k];
    }
  }
}

/// @brief Ring multiplication followed directly by rounding from R_q to R_p.
void ring_mul_Awin_round_p(poly *r,
                           const ring_element_Awin *a,
                           const ring_element *b,
                           int ncoeffs)
{
  int row_min = RRLWR_K - ncoeffs;
  uint16_t acc[RRLWR_N];

  for(int i = RRLWR_K - 1; i >= row_min; i--) {
    int out = i - row_min;
    const poly *row = &a->x[RRLWR_K - 1 - i];

    poly_mul_toom4_u16(acc, &row[0], &b->x[0]);
    for(int j = 1; j < RRLWR_K; j++) {
      poly_macc_toom4_u16(acc, &row[j], &b->x[j]);
    }

    for(unsigned int k = 0; k < RRLWR_N; k++) {
      r[out].coeffs[k] = round_q_to_p_from_u16(acc[k]);
    }
  }
}

void ring_mul_Awin_add_msg_pack_t(unsigned char *ct,
                                  const ring_element_Awin *a,
                                  const ring_element *b,
                                  const unsigned char msg[RRLWR_PKE_MESSAGE_LEN])
{
  int row_min = RRLWR_K - RRLWR_PKE_ELL;
  uint16_t acc[RRLWR_N];

  for(int i = RRLWR_K - 1; i >= row_min; i--) {
    unsigned int out = (unsigned int)(i - row_min);
    const poly *row = &a->x[RRLWR_K - 1 - i];

    poly_mul_toom4_u16(acc, &row[0], &b->x[0]);
    for(int j = 1; j < RRLWR_K; j++) {
      poly_macc_toom4_u16(acc, &row[j], &b->x[j]);
    }

    poly_pack_ciphertext_t_from_acc_msg(ct + out * RRLWR_PKE_PACKED_POLYT_LEN,
                                        acc,
                                        msg,
                                        out);
  }
}

/// @brief Ring multiplication over R_q with schoolbook polynomial products.
void ring_mul(poly *r, const ring_element *a, const ring_element *b, int ncoeffs)
{
  ring_element_Awin aw;

  ring_to_Awin(&aw, a);
  ring_mul_Awin(r, &aw, b, ncoeffs);
}

void ring_round_xtoy(ring_element *r, const ring_element *f, int32_t x, int32_t y) {
  for(unsigned int i = 0; i < RRLWR_K; i++) {
    poly_round_xtoy(&r->x[i], &f->x[i], x, y);
  }
}
