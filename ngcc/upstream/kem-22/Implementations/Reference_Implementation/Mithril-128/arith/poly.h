#ifndef POLY_H
#define POLY_H

#include "parameters.h"

#ifdef __cplusplus
extern "C"
{
#endif

  typedef struct{
    uint16_t coeffs[RRLWR_N];
  } poly;

  void poly_mul_toom4_u16(uint16_t r[RRLWR_N], const poly *f, const poly *g);
  void poly_macc_toom4_u16(uint16_t r[RRLWR_N], const poly *f, const poly *g);
  void poly_mul_toom4(poly *r, const poly *f, const poly *g);
  void poly_mul_schoolbook(poly *r, const poly *f, const poly *g);
  void poly_add(poly *r, poly *f, poly *g);
  void poly_sub(poly *r, poly *f, poly *g);
  void poly_reduce_pow2(poly *r, poly *f, int32_t d);
  void poly_round_xtoy(poly *r, const poly *f, int32_t x, int32_t y);
  void poly_compress(poly *r, int32_t x);
  void poly_decompress(poly *r, int32_t x);

#ifdef __cplusplus
}
#endif

#endif
