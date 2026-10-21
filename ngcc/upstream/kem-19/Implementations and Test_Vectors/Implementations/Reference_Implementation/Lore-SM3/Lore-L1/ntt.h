#ifndef NTT_H
#define NTT_H

#include <stdint.h>
#include "params.h"

extern int16_t zetas[128];

int16_t fqmul(int16_t a, int16_t b);

void ntt(int16_t r[LORE_N]);
void invntt_tomont(int16_t r[LORE_N]);
void poly_mul_ntt(int16_t r[LORE_N], const int16_t a[LORE_N], const int16_t b[LORE_N]);

#endif