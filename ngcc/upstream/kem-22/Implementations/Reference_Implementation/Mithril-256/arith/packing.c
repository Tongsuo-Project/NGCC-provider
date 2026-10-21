#include "packing.h"

#ifndef RRLWR_PACKING_16BIT_SCALAR
#define RRLWR_PACKING_16BIT_SCALAR 0
#endif

#if RRLWR_PACKING_16BIT_SCALAR
typedef uint16_t pack_scalar_t;
#define PACK_SCALAR(x) ((uint16_t)(x))
#define PACK_ACC(x) (x)
#define PACK_BYTE(x) (x)
#else
typedef uint32_t pack_scalar_t;
#define PACK_SCALAR(x) ((uint32_t)(x))
#define PACK_ACC(x) ((uint32_t)(x))
#define PACK_BYTE(x) ((uint32_t)(x))
#endif

static void poly_pack_2(unsigned char *b, const poly *r)
{
  for(unsigned int i = 0; i < RRLWR_N / 4; i++) {
    pack_scalar_t c0 = PACK_SCALAR((1 - r->coeffs[4 * i + 0]) & 0x3);
    pack_scalar_t c1 = PACK_SCALAR((1 - r->coeffs[4 * i + 1]) & 0x3);
    pack_scalar_t c2 = PACK_SCALAR((1 - r->coeffs[4 * i + 2]) & 0x3);
    pack_scalar_t c3 = PACK_SCALAR((1 - r->coeffs[4 * i + 3]) & 0x3);

    b[i] = (unsigned char)(c0 | (c1 << 2) | (c2 << 4) | (c3 << 6));
  }
}

static void poly_pack_11(unsigned char *b, const poly *r)
{
  for(unsigned int i = 0; i < RRLWR_N / 8; i++) {
    pack_scalar_t c0 = PACK_SCALAR((1023 - r->coeffs[8 * i + 0]) & 0x7ff);
    pack_scalar_t c1 = PACK_SCALAR((1023 - r->coeffs[8 * i + 1]) & 0x7ff);
    pack_scalar_t c2 = PACK_SCALAR((1023 - r->coeffs[8 * i + 2]) & 0x7ff);
    pack_scalar_t c3 = PACK_SCALAR((1023 - r->coeffs[8 * i + 3]) & 0x7ff);
    pack_scalar_t c4 = PACK_SCALAR((1023 - r->coeffs[8 * i + 4]) & 0x7ff);
    pack_scalar_t c5 = PACK_SCALAR((1023 - r->coeffs[8 * i + 5]) & 0x7ff);
    pack_scalar_t c6 = PACK_SCALAR((1023 - r->coeffs[8 * i + 6]) & 0x7ff);
    pack_scalar_t c7 = PACK_SCALAR((1023 - r->coeffs[8 * i + 7]) & 0x7ff);
    unsigned int p = 11 * i;

    b[p + 0] = (unsigned char)c0;
    b[p + 1] = (unsigned char)((c0 >> 8) | (c1 << 3));
    b[p + 2] = (unsigned char)((c1 >> 5) | (c2 << 6));
    b[p + 3] = (unsigned char)(c2 >> 2);
    b[p + 4] = (unsigned char)((c2 >> 10) | (c3 << 1));
    b[p + 5] = (unsigned char)((c3 >> 7) | (c4 << 4));
    b[p + 6] = (unsigned char)((c4 >> 4) | (c5 << 7));
    b[p + 7] = (unsigned char)(c5 >> 1);
    b[p + 8] = (unsigned char)((c5 >> 9) | (c6 << 2));
    b[p + 9] = (unsigned char)((c6 >> 6) | (c7 << 5));
    b[p + 10] = (unsigned char)(c7 >> 3);
  }
}

static void poly_unpack_2(poly *r, const unsigned char *b)
{
  const pack_scalar_t qmask = RRLWR_PKE_Q - 1;

  for(unsigned int i = 0; i < RRLWR_N / 4; i++) {
    pack_scalar_t x = b[i];

    r->coeffs[4 * i + 0] = (uint16_t)((1 - (x & 0x3)) & qmask);
    r->coeffs[4 * i + 1] = (uint16_t)((1 - ((x >> 2) & 0x3)) & qmask);
    r->coeffs[4 * i + 2] = (uint16_t)((1 - ((x >> 4) & 0x3)) & qmask);
    r->coeffs[4 * i + 3] = (uint16_t)((1 - (x >> 6)) & qmask);
  }
}

static void poly_unpack_11(poly *r, const unsigned char *b)
{
  for(unsigned int i = 0; i < RRLWR_N / 8; i++) {
    const unsigned char *p = b + 11 * i;
    pack_scalar_t c0 = PACK_SCALAR((PACK_BYTE(p[0]) | (PACK_BYTE(p[1]) << 8)) & 0x7ff);
    pack_scalar_t c1 = PACK_SCALAR(((PACK_BYTE(p[1]) >> 3) | (PACK_BYTE(p[2]) << 5)) & 0x7ff);
    pack_scalar_t c2 = PACK_SCALAR(((PACK_BYTE(p[2]) >> 6) | (PACK_BYTE(p[3]) << 2) |
                                    (PACK_BYTE(p[4]) << 10)) & 0x7ff);
    pack_scalar_t c3 = PACK_SCALAR(((PACK_BYTE(p[4]) >> 1) | (PACK_BYTE(p[5]) << 7)) & 0x7ff);
    pack_scalar_t c4 = PACK_SCALAR(((PACK_BYTE(p[5]) >> 4) | (PACK_BYTE(p[6]) << 4)) & 0x7ff);
    pack_scalar_t c5 = PACK_SCALAR(((PACK_BYTE(p[6]) >> 7) | (PACK_BYTE(p[7]) << 1) |
                                    (PACK_BYTE(p[8]) << 9)) & 0x7ff);
    pack_scalar_t c6 = PACK_SCALAR(((PACK_BYTE(p[8]) >> 2) | (PACK_BYTE(p[9]) << 6)) & 0x7ff);
    pack_scalar_t c7 = PACK_SCALAR(((PACK_BYTE(p[9]) >> 5) | (PACK_BYTE(p[10]) << 3)) & 0x7ff);

    r->coeffs[8 * i + 0] = (uint16_t)((1023 - c0) & 0x7ff);
    r->coeffs[8 * i + 1] = (uint16_t)((1023 - c1) & 0x7ff);
    r->coeffs[8 * i + 2] = (uint16_t)((1023 - c2) & 0x7ff);
    r->coeffs[8 * i + 3] = (uint16_t)((1023 - c3) & 0x7ff);
    r->coeffs[8 * i + 4] = (uint16_t)((1023 - c4) & 0x7ff);
    r->coeffs[8 * i + 5] = (uint16_t)((1023 - c5) & 0x7ff);
    r->coeffs[8 * i + 6] = (uint16_t)((1023 - c6) & 0x7ff);
    r->coeffs[8 * i + 7] = (uint16_t)((1023 - c7) & 0x7ff);
  }
}

static void poly_unpack_13(poly *r, const unsigned char *b)
{
  for(unsigned int i = 0; i < RRLWR_N / 8; i++) {
    const unsigned char *p = b + 13 * i;
    pack_scalar_t c0 = PACK_SCALAR((PACK_BYTE(p[0]) | (PACK_BYTE(p[1]) << 8)) & 0x1fff);
    pack_scalar_t c1 = PACK_SCALAR(((PACK_BYTE(p[1]) >> 5) | (PACK_BYTE(p[2]) << 3) |
                                    (PACK_BYTE(p[3]) << 11)) & 0x1fff);
    pack_scalar_t c2 = PACK_SCALAR(((PACK_BYTE(p[3]) >> 2) | (PACK_BYTE(p[4]) << 6)) & 0x1fff);
    pack_scalar_t c3 = PACK_SCALAR(((PACK_BYTE(p[4]) >> 7) | (PACK_BYTE(p[5]) << 1) |
                                    (PACK_BYTE(p[6]) << 9)) & 0x1fff);
    pack_scalar_t c4 = PACK_SCALAR(((PACK_BYTE(p[6]) >> 4) | (PACK_BYTE(p[7]) << 4) |
                                    (PACK_BYTE(p[8]) << 12)) & 0x1fff);
    pack_scalar_t c5 = PACK_SCALAR(((PACK_BYTE(p[8]) >> 1) | (PACK_BYTE(p[9]) << 7)) & 0x1fff);
    pack_scalar_t c6 = PACK_SCALAR(((PACK_BYTE(p[9]) >> 6) | (PACK_BYTE(p[10]) << 2) |
                                    (PACK_BYTE(p[11]) << 10)) & 0x1fff);
    pack_scalar_t c7 = PACK_SCALAR(((PACK_BYTE(p[11]) >> 3) | (PACK_BYTE(p[12]) << 5)) & 0x1fff);

    r->coeffs[8 * i + 0] = (uint16_t)((4095 - c0) & 0x1fff);
    r->coeffs[8 * i + 1] = (uint16_t)((4095 - c1) & 0x1fff);
    r->coeffs[8 * i + 2] = (uint16_t)((4095 - c2) & 0x1fff);
    r->coeffs[8 * i + 3] = (uint16_t)((4095 - c3) & 0x1fff);
    r->coeffs[8 * i + 4] = (uint16_t)((4095 - c4) & 0x1fff);
    r->coeffs[8 * i + 5] = (uint16_t)((4095 - c5) & 0x1fff);
    r->coeffs[8 * i + 6] = (uint16_t)((4095 - c6) & 0x1fff);
    r->coeffs[8 * i + 7] = (uint16_t)((4095 - c7) & 0x1fff);
  }
}

#if RRLWR_PKE_LOGT != 3 && RRLWR_PKE_LOGT != 8
static uint32_t encrypt_t_coeff(uint16_t x,
                                const unsigned char *msg,
                                unsigned int out,
                                unsigned int k)
{
  pack_scalar_t mj = PACK_SCALAR((msg[out * RRLWR_N / 8 + (k >> 3)] >> (k & 0x7)) & 1);
  pack_scalar_t c = x;

  c = PACK_SCALAR((c + (1 << (RRLWR_PKE_LOGQ - (RRLWR_PKE_LOGP + 1))) +
       (-mj & (1 << (RRLWR_PKE_LOGP - 1)))) &
      (RRLWR_PKE_P - 1));
  c = PACK_SCALAR(c >> (RRLWR_PKE_LOGP - RRLWR_PKE_LOGT));
  c = PACK_SCALAR((((1 << (RRLWR_PKE_LOGT - 1)) - 1) - c) &
      ((1 << RRLWR_PKE_LOGT) - 1));

  return c;
}
#endif

void poly_pack_ciphertext_t_from_acc_msg(unsigned char *ct,
                                         const uint16_t acc[RRLWR_N],
                                         const unsigned char msg[RRLWR_PKE_MESSAGE_LEN],
                                         unsigned int out)
{
#if RRLWR_PKE_LOGT == 3
  for(unsigned int i = 0; i < RRLWR_N / 8; i++) {
    unsigned int k = 8 * i;
    pack_scalar_t m = msg[out * RRLWR_N / 8 + i];
    pack_scalar_t c0 = PACK_SCALAR((3 - (((PACK_ACC(acc[k + 0]) + 2) & 0x7ff) >> 8) - ((m & 1) << 2)) & 7);
    pack_scalar_t c1 = PACK_SCALAR((3 - (((PACK_ACC(acc[k + 1]) + 2) & 0x7ff) >> 8) - (((m >> 1) & 1) << 2)) & 7);
    pack_scalar_t c2 = PACK_SCALAR((3 - (((PACK_ACC(acc[k + 2]) + 2) & 0x7ff) >> 8) - (((m >> 2) & 1) << 2)) & 7);
    pack_scalar_t c3 = PACK_SCALAR((3 - (((PACK_ACC(acc[k + 3]) + 2) & 0x7ff) >> 8) - (((m >> 3) & 1) << 2)) & 7);
    pack_scalar_t c4 = PACK_SCALAR((3 - (((PACK_ACC(acc[k + 4]) + 2) & 0x7ff) >> 8) - (((m >> 4) & 1) << 2)) & 7);
    pack_scalar_t c5 = PACK_SCALAR((3 - (((PACK_ACC(acc[k + 5]) + 2) & 0x7ff) >> 8) - (((m >> 5) & 1) << 2)) & 7);
    pack_scalar_t c6 = PACK_SCALAR((3 - (((PACK_ACC(acc[k + 6]) + 2) & 0x7ff) >> 8) - (((m >> 6) & 1) << 2)) & 7);
    pack_scalar_t c7 = PACK_SCALAR((3 - (((PACK_ACC(acc[k + 7]) + 2) & 0x7ff) >> 8) - ((m >> 7) << 2)) & 7);
    unsigned int p = 3 * i;

    ct[p + 0] = (unsigned char)(c0 | (c1 << 3) | (c2 << 6));
    ct[p + 1] = (unsigned char)((c2 >> 2) | (c3 << 1) | (c4 << 4) | (c5 << 7));
    ct[p + 2] = (unsigned char)((c5 >> 1) | (c6 << 2) | (c7 << 5));
  }
#elif RRLWR_PKE_LOGT == 8
  for(unsigned int i = 0; i < RRLWR_N / 8; i++) {
    unsigned int k = 8 * i;
    pack_scalar_t m = msg[out * RRLWR_N / 8 + i];

    ct[k + 0] = (unsigned char)((127 - (((PACK_ACC(acc[k + 0]) + 2) & 0x7ff) >> 3) - ((m & 1) << 7)) & 0xff);
    ct[k + 1] = (unsigned char)((127 - (((PACK_ACC(acc[k + 1]) + 2) & 0x7ff) >> 3) - (((m >> 1) & 1) << 7)) & 0xff);
    ct[k + 2] = (unsigned char)((127 - (((PACK_ACC(acc[k + 2]) + 2) & 0x7ff) >> 3) - (((m >> 2) & 1) << 7)) & 0xff);
    ct[k + 3] = (unsigned char)((127 - (((PACK_ACC(acc[k + 3]) + 2) & 0x7ff) >> 3) - (((m >> 3) & 1) << 7)) & 0xff);
    ct[k + 4] = (unsigned char)((127 - (((PACK_ACC(acc[k + 4]) + 2) & 0x7ff) >> 3) - (((m >> 4) & 1) << 7)) & 0xff);
    ct[k + 5] = (unsigned char)((127 - (((PACK_ACC(acc[k + 5]) + 2) & 0x7ff) >> 3) - (((m >> 5) & 1) << 7)) & 0xff);
    ct[k + 6] = (unsigned char)((127 - (((PACK_ACC(acc[k + 6]) + 2) & 0x7ff) >> 3) - (((m >> 6) & 1) << 7)) & 0xff);
    ct[k + 7] = (unsigned char)((127 - (((PACK_ACC(acc[k + 7]) + 2) & 0x7ff) >> 3) - ((m >> 7) << 7)) & 0xff);
  }
#else
  unsigned int acc_shift = 0;
  unsigned int bpos = 0;
  uint32_t pack_acc = 0;

  for(unsigned int k = 0; k < RRLWR_N; k++) {
    pack_acc |= encrypt_t_coeff(acc[k], msg, out, k) << acc_shift;
    acc_shift += RRLWR_PKE_LOGT;
    while(acc_shift >= 8) {
      ct[bpos++] = (unsigned char)(pack_acc & 0xff);
      pack_acc >>= 8;
      acc_shift -= 8;
    }
  }
#endif
}

/// @brief Pack a polynomial with coefficients in [-bitlen/2, bitlen/2-1] into a byte string of bitlen bits per coefficient
void poly_pack(unsigned char *b, poly *r, int32_t bitlen) {
  if(bitlen == 2) {
    poly_pack_2(b, r);
    return;
  }

  if(bitlen == 11) {
    poly_pack_11(b, r);
    return;
  }

  unsigned int i;
  unsigned int acc_shift = 0; 
  unsigned int bpos = 0;
  poly rp;
  uint16_t *rc = rp.coeffs;
  uint32_t acc = 0;

  // Make all coefficients from r positive
  for(i = 0; i < RRLWR_N; i++) {
    rp.coeffs[i] = (uint16_t)(((((int32_t)1 << (bitlen - 1)) - 1) -
                               (int32_t)r->coeffs[i]) &
                              (((int32_t)1 << bitlen) - 1));
  }

  while (rc < rp.coeffs + RRLWR_N) {

      // Main packing loop
      for(i = 0; i < 32; i++) {
        acc |= ((uint32_t)rc[i]) << acc_shift; // Take the next bitlen bits
        acc_shift += bitlen;
        while (acc_shift >= 8) {
          b[bpos++] = (unsigned char)(acc & 0xFF);
          acc >>= 8;
          acc_shift -= 8;
        }
      }

    rc += 32; // Unpack 32 coefficients at a time
  }
}

static void ring_pack_11(unsigned char *b, const ring_element *r)
{
  for(unsigned int i = 0; i < RRLWR_K; i++) {
    poly_pack_11(b + i * RRLWR_PKE_PACKED_POLYP_LEN, &r->x[i]);
  }
}

static void ring_pack_2(unsigned char *b, const ring_element *r)
{
  for(unsigned int i = 0; i < RRLWR_K; i++) {
    poly_pack_2(b + i * RRLWR_PACKED_POLY2_LEN, &r->x[i]);
  }
}

void ring_pack(unsigned char *b, ring_element *r, int32_t bitlen) {
  if(bitlen == 2) {
    ring_pack_2(b, r);
    return;
  }

  if(bitlen == 11) {
    ring_pack_11(b, r);
    return;
  }

  unsigned int offset = bitlen*(RRLWR_N>>3);
  for (unsigned int i = 0; i < RRLWR_K; i++) {
    poly_pack(b+i*offset, &r->x[i], bitlen);
  }
}

/// @brief Unpack a polynomial with coefficients in [-bitlen/2, bitlen/2-1] from a byte string of bitlen bits per coefficient
void poly_unpack(poly *r, const unsigned char *b, int32_t bitlen) {
  if(bitlen == 2) {
    poly_unpack_2(r, b);
    return;
  }

  if(bitlen == 13) {
    poly_unpack_13(r, b);
    return;
  }

  if(bitlen == 11) {
    poly_unpack_11(r, b);
    return;
  }

  unsigned int i;
  int32_t acc_shift = 0; 
  unsigned int bpos = 0;
  uint16_t *rc = r->coeffs;
  uint32_t acc = 0;

  while (rc < r->coeffs + RRLWR_N) {

      // Main unpacking loop
      for(i = 0; i < 32; i++) {
        while (acc_shift < bitlen) {
          acc |= ((uint32_t)b[bpos++]) << acc_shift;
          acc_shift += 8;
        }
        rc[i] = (uint16_t)(acc & (((int32_t)1 << bitlen)-1));
        acc >>= bitlen;
        acc_shift -= bitlen;
      }

    rc += 32; // Unpack 32 coefficients at a time
  }

  // Convert packed coefficients to low-bit residues.
  for(i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] = (uint16_t)(((((int32_t)1 << (bitlen - 1)) - 1) -
                               (int32_t)r->coeffs[i]) &
                              (((int32_t)1 << bitlen) - 1));
  }
}

static void ring_unpack_2(ring_element *r, const unsigned char *b)
{
  for(unsigned int i = 0; i < RRLWR_K; i++) {
    poly_unpack_2(&r->x[i], b + i * RRLWR_PACKED_POLY2_LEN);
  }
}

void ring_unpack(ring_element *r, const unsigned char *b, int32_t bitlen) {
  if(bitlen == 2) {
    ring_unpack_2(r, b);
    return;
  }

  unsigned int offset = bitlen*(RRLWR_N>>3);
  for (unsigned int i = 0; i < RRLWR_K; i++) {
    poly_unpack(&r->x[i], b+i*offset, bitlen);
  }
}
