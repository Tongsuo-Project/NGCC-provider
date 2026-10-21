#include "poly.h"

#include <string.h>

#define TOOM4_N (RRLWR_N)
#define TOOM4_BLK (RRLWR_N / 4)
#define TOOM4_RES (2 * TOOM4_BLK - 1)
#define TOOM4_FULL_RES (2 * RRLWR_N)

static void karatsuba32_simple_u16(const uint16_t *a,
                                   const uint16_t *b,
                                   uint16_t out[TOOM4_RES])
{
  uint16_t d01[TOOM4_BLK / 2 - 1];
  uint16_t d0123[TOOM4_BLK / 2 - 1];
  uint16_t d23[TOOM4_BLK / 2 - 1];
  uint16_t result_d01[TOOM4_BLK - 1];

  memset(out, 0, TOOM4_RES * sizeof(uint16_t));
  memset(result_d01, 0, (TOOM4_BLK - 1) * sizeof(uint16_t));
  memset(d01, 0, (TOOM4_BLK / 2 - 1) * sizeof(uint16_t));
  memset(d0123, 0, (TOOM4_BLK / 2 - 1) * sizeof(uint16_t));
  memset(d23, 0, (TOOM4_BLK / 2 - 1) * sizeof(uint16_t));

  for(unsigned int i = 0; i < TOOM4_BLK / 4; i++) {
    uint16_t a0 = a[i];
    uint16_t a1 = a[i + TOOM4_BLK / 4];
    uint16_t a2 = a[i + 2 * TOOM4_BLK / 4];
    uint16_t a3 = a[i + 3 * TOOM4_BLK / 4];

    for(unsigned int j = 0; j < TOOM4_BLK / 4; j++) {
      uint16_t b0 = b[j];
      uint16_t b1 = b[j + TOOM4_BLK / 4];
      uint16_t b2 = b[j + 2 * TOOM4_BLK / 4];
      uint16_t b3 = b[j + 3 * TOOM4_BLK / 4];
      uint16_t b01 = (uint16_t)(b0 + b1);
      uint16_t a01 = (uint16_t)(a0 + a1);
      uint16_t b23 = (uint16_t)(b2 + b3);
      uint16_t a23 = (uint16_t)(a2 + a3);
      uint16_t b02 = (uint16_t)(b0 + b2);
      uint16_t a02 = (uint16_t)(a0 + a2);
      uint16_t b13 = (uint16_t)(b1 + b3);
      uint16_t a13 = (uint16_t)(a1 + a3);

      out[i + j] = (uint16_t)(out[i + j] + (uint16_t)((uint32_t)a0 * b0));
      out[i + j + 2 * TOOM4_BLK / 4] =
          (uint16_t)(out[i + j + 2 * TOOM4_BLK / 4] + (uint16_t)((uint32_t)a1 * b1));
      out[i + j + 4 * TOOM4_BLK / 4] =
          (uint16_t)(out[i + j + 4 * TOOM4_BLK / 4] + (uint16_t)((uint32_t)a2 * b2));
      out[i + j + 6 * TOOM4_BLK / 4] =
          (uint16_t)(out[i + j + 6 * TOOM4_BLK / 4] + (uint16_t)((uint32_t)a3 * b3));

      d01[i + j] = (uint16_t)(d01[i + j] + (uint16_t)((uint32_t)a01 * b01));
      d23[i + j] = (uint16_t)(d23[i + j] + (uint16_t)((uint32_t)a23 * b23));

      result_d01[i + j] =
          (uint16_t)(result_d01[i + j] + (uint16_t)((uint32_t)a02 * b02));
      result_d01[i + j + 2 * TOOM4_BLK / 4] =
          (uint16_t)(result_d01[i + j + 2 * TOOM4_BLK / 4] +
                     (uint16_t)((uint32_t)a13 * b13));

      d0123[i + j] =
          (uint16_t)(d0123[i + j] +
                     (uint16_t)((uint32_t)(a02 + a13) * (uint16_t)(b02 + b13)));
    }
  }

  for(unsigned int i = 0; i < TOOM4_BLK / 2 - 1; i++) {
    d0123[i] = (uint16_t)(d0123[i] - result_d01[i] -
                          result_d01[i + 2 * TOOM4_BLK / 4]);
    d01[i] = (uint16_t)(d01[i] - out[i] - out[i + 2 * TOOM4_BLK / 4]);
    d23[i] = (uint16_t)(d23[i] - out[i + 4 * TOOM4_BLK / 4] -
                        out[i + 6 * TOOM4_BLK / 4]);
  }

  for(unsigned int i = 0; i < TOOM4_BLK / 2 - 1; i++) {
    result_d01[i + TOOM4_BLK / 4] =
        (uint16_t)(result_d01[i + TOOM4_BLK / 4] + d0123[i]);
    out[i + TOOM4_BLK / 4] = (uint16_t)(out[i + TOOM4_BLK / 4] + d01[i]);
    out[i + 5 * TOOM4_BLK / 4] =
        (uint16_t)(out[i + 5 * TOOM4_BLK / 4] + d23[i]);
  }

  for(unsigned int i = 0; i < TOOM4_BLK - 1; i++) {
    result_d01[i] = (uint16_t)(result_d01[i] - out[i] - out[i + TOOM4_BLK]);
  }

  for(unsigned int i = 0; i < TOOM4_BLK - 1; i++) {
    out[i + TOOM4_BLK / 2] = (uint16_t)(out[i + TOOM4_BLK / 2] + result_d01[i]);
  }
}

static void toom4_128_fold_u16(const uint16_t a[TOOM4_N],
                               const uint16_t b[TOOM4_N],
                               uint16_t r[TOOM4_N],
                               int clear)
{
  const uint16_t inv3 = 43691;
  const uint16_t inv9 = 36409;
  const uint16_t inv15 = 61167;
  uint16_t aw1[TOOM4_BLK], aw2[TOOM4_BLK], aw3[TOOM4_BLK], aw4[TOOM4_BLK];
  uint16_t aw5[TOOM4_BLK], aw6[TOOM4_BLK], aw7[TOOM4_BLK];
  uint16_t bw1[TOOM4_BLK], bw2[TOOM4_BLK], bw3[TOOM4_BLK], bw4[TOOM4_BLK];
  uint16_t bw5[TOOM4_BLK], bw6[TOOM4_BLK], bw7[TOOM4_BLK];
  uint16_t w1[TOOM4_RES], w2[TOOM4_RES], w3[TOOM4_RES], w4[TOOM4_RES];
  uint16_t w5[TOOM4_RES], w6[TOOM4_RES], w7[TOOM4_RES];

  if(clear) {
    memset(r, 0, TOOM4_N * sizeof(uint16_t));
  }

  for(unsigned int j = 0; j < TOOM4_BLK; j++) {
    uint16_t r0 = a[j];
    uint16_t r1 = a[TOOM4_BLK + j];
    uint16_t r2 = a[2 * TOOM4_BLK + j];
    uint16_t r3 = a[3 * TOOM4_BLK + j];
    uint16_t r4 = (uint16_t)(r0 + r2);
    uint16_t r5 = (uint16_t)(r1 + r3);

    aw3[j] = (uint16_t)(r4 + r5);
    aw4[j] = (uint16_t)(r4 - r5);
    r4 = (uint16_t)(((r0 << 2) + r2) << 1);
    r5 = (uint16_t)((r1 << 2) + r3);
    aw5[j] = (uint16_t)(r4 + r5);
    aw6[j] = (uint16_t)(r4 - r5);
    aw2[j] = (uint16_t)((r3 << 3) + (r2 << 2) + (r1 << 1) + r0);
    aw7[j] = r0;
    aw1[j] = r3;

    r0 = b[j];
    r1 = b[TOOM4_BLK + j];
    r2 = b[2 * TOOM4_BLK + j];
    r3 = b[3 * TOOM4_BLK + j];
    r4 = (uint16_t)(r0 + r2);
    r5 = (uint16_t)(r1 + r3);

    bw3[j] = (uint16_t)(r4 + r5);
    bw4[j] = (uint16_t)(r4 - r5);
    r4 = (uint16_t)(((r0 << 2) + r2) << 1);
    r5 = (uint16_t)((r1 << 2) + r3);
    bw5[j] = (uint16_t)(r4 + r5);
    bw6[j] = (uint16_t)(r4 - r5);
    bw2[j] = (uint16_t)((r3 << 3) + (r2 << 2) + (r1 << 1) + r0);
    bw7[j] = r0;
    bw1[j] = r3;
  }

  karatsuba32_simple_u16(aw1, bw1, w1);
  karatsuba32_simple_u16(aw2, bw2, w2);
  karatsuba32_simple_u16(aw3, bw3, w3);
  karatsuba32_simple_u16(aw4, bw4, w4);
  karatsuba32_simple_u16(aw5, bw5, w5);
  karatsuba32_simple_u16(aw6, bw6, w6);
  karatsuba32_simple_u16(aw7, bw7, w7);

  for(unsigned int i = 0; i < TOOM4_RES; i++) {
    uint16_t r0 = w1[i];
    uint16_t r1 = w2[i];
    uint16_t r2 = w3[i];
    uint16_t r3 = w4[i];
    uint16_t r4 = w5[i];
    uint16_t r5 = w6[i];
    uint16_t r6 = w7[i];

    r1 = (uint16_t)(r1 + r4);
    r5 = (uint16_t)(r5 - r4);
    r3 = (uint16_t)((r3 - r2) >> 1);
    r4 = (uint16_t)(r4 - r0);
    r4 = (uint16_t)(r4 - (r6 << 6));
    r4 = (uint16_t)((r4 << 1) + r5);
    r2 = (uint16_t)(r2 + r3);
    r1 = (uint16_t)(r1 - (r2 << 6) - r2);
    r2 = (uint16_t)(r2 - r6);
    r2 = (uint16_t)(r2 - r0);
    r1 = (uint16_t)(r1 + 45 * r2);
    r4 = (uint16_t)(((r4 - (r2 << 3)) * (uint32_t)inv3) >> 3);
    r5 = (uint16_t)(r5 + r1);
    r1 = (uint16_t)(((r1 + (r3 << 4)) * (uint32_t)inv9) >> 1);
    r3 = (uint16_t)(-(uint16_t)(r3 + r1));
    r5 = (uint16_t)(((30 * r1 - r5) * (uint32_t)inv15) >> 2);
    r2 = (uint16_t)(r2 - r4);
    r1 = (uint16_t)(r1 - r5);

    r[i] = (uint16_t)(r[i] + r6 - r2);
    r[i + TOOM4_BLK] = (uint16_t)(r[i + TOOM4_BLK] + r5 - r1);
    r[i + 2 * TOOM4_BLK] = (uint16_t)(r[i + 2 * TOOM4_BLK] + r4 - r0);
    if(i < TOOM4_BLK) {
      r[i + 3 * TOOM4_BLK] = (uint16_t)(r[i + 3 * TOOM4_BLK] + r3);
    } else {
      r[i - TOOM4_BLK] = (uint16_t)(r[i - TOOM4_BLK] - r3);
    }
  }
}

void poly_macc_toom4_u16(uint16_t r[RRLWR_N], const poly *f, const poly *g)
{
  toom4_128_fold_u16(f->coeffs, g->coeffs, r, 0);
}

void poly_mul_toom4_u16(uint16_t r[RRLWR_N], const poly *f, const poly *g)
{
  toom4_128_fold_u16(f->coeffs, g->coeffs, r, 1);
}

void poly_mul_toom4(poly *r, const poly *f, const poly *g)
{
  uint16_t folded[TOOM4_N];

  poly_mul_toom4_u16(folded, f, g);

  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] = folded[i];
  }
}

void poly_mul_schoolbook(poly *r, const poly *f, const poly *g)
{
  uint16_t acc[RRLWR_N] = {0};

  for(unsigned int i = 0; i < RRLWR_N; i++) {
    for(unsigned int j = 0; j < RRLWR_N; j++) {
      uint16_t prod = (uint16_t)((uint32_t)f->coeffs[i] * g->coeffs[j]);
      unsigned int d = i + j;

      if(d < RRLWR_N) {
        acc[d] = (uint16_t)(acc[d] + prod);
      } else {
        acc[d - RRLWR_N] = (uint16_t)(acc[d - RRLWR_N] - prod);
      }
    }
  }

  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] = acc[i];
  }
}

void poly_add(poly *r, poly *f, poly *g) {
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] = (uint16_t)(f->coeffs[i] + g->coeffs[i]);
  }
}

void poly_sub(poly *r, poly *f, poly *g) {
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] = (uint16_t)(f->coeffs[i] - g->coeffs[i]);
  }
}

/// @brief Reduce the input modulo 2**d into residue interval [0, 2**d-1]
void poly_reduce_pow2(poly *r, poly *f, int32_t d) {
  int32_t pow2 = (int32_t)1 << d;         // 2^d
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] = (uint16_t)(f->coeffs[i] & (pow2 - 1));
  }
}

void poly_round_xtoy(poly *r, const poly *f, int32_t x, int32_t y) {
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    int32_t c = (int16_t)(uint16_t)(f->coeffs[i] << (16 - x));
    c >>= 16 - x;
    c += (int32_t)1 << (x-(y+1));                         // Add constant x/(2*y)
    c >>= (x-y);                                          // Divide by x/y and floor
    c &= ((int32_t)1 << y)-1;                             // Reduce mod y
    r->coeffs[i] = (uint16_t)c;
  }
}

void poly_compress(poly *r, int32_t x) {
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] = (uint16_t)(r->coeffs[i] >> x);
  }
}

void poly_decompress(poly *r, int32_t x) {
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] = (uint16_t)(r->coeffs[i] << x);
  }
}
