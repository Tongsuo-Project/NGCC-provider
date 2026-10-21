/*
 * Independent arithmetic correctness tests for the Toom-Cook multiplication
 * path. The oracle below intentionally does not call poly_mul_schoolbook() or
 * any existing test helper, so it can catch shared mistakes in implementation
 * and older unit tests.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "parameters.h"
#include "packing.h"
#include "pke.h"
#include "poly.h"
#include "ring.h"
#include "uniform.h"

#define RANDOM_POLY_TESTS 512
#define RANDOM_RING_TESTS 64
#define RANDOM_PACKING_TESTS 64
#define RANDOM_UNIFORM_TESTS 16

static uint32_t rng_state = 0x9e3779b9u;

static uint32_t next_u32(void)
{
  uint32_t x = rng_state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  rng_state = x;
  return x;
}

static uint16_t next_coeff(void)
{
  uint32_t x = next_u32();

  switch(x & 7u) {
  case 0:
    return 0;
  case 1:
    return 1;
  case 2:
    return (uint16_t)(RRLWR_PKE_Q - 1);
  case 3:
    return (uint16_t)(RRLWR_PKE_Q / 2);
  default:
    return (uint16_t)(x & 0xffffu);
  }
}

static void random_poly(poly *r)
{
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] = next_coeff();
  }
}

static void random_ring(ring_element *r)
{
  for(unsigned int i = 0; i < RRLWR_K; i++) {
    random_poly(&r->x[i]);
  }
}

static void oracle_poly_mul_acc(uint16_t r[RRLWR_N],
                                const poly *a,
                                const poly *b,
                                int clear)
{
  uint64_t full[2 * RRLWR_N - 1] = {0};

  if(clear) {
    memset(r, 0, RRLWR_N * sizeof(uint16_t));
  }

  for(unsigned int i = 0; i < RRLWR_N; i++) {
    for(unsigned int j = 0; j < RRLWR_N; j++) {
      full[i + j] += (uint64_t)a->coeffs[i] * b->coeffs[j];
    }
  }

  for(unsigned int i = 0; i < RRLWR_N; i++) {
    uint16_t lo = (uint16_t)full[i];
    uint16_t hi = 0;

    if(i + RRLWR_N < 2 * RRLWR_N - 1) {
      hi = (uint16_t)full[i + RRLWR_N];
    }

    r[i] = (uint16_t)(r[i] + lo - hi);
  }
}

static void oracle_poly_mul(poly *r, const poly *a, const poly *b)
{
  oracle_poly_mul_acc(r->coeffs, a, b, 1);
}

static void oracle_poly_addto(poly *r, const poly *a)
{
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] = (uint16_t)(r->coeffs[i] + a->coeffs[i]);
  }
}

static void oracle_poly_mul_x_plus_2(poly *r, const poly *a)
{
  r->coeffs[0] = (uint16_t)(2 * a->coeffs[0] - a->coeffs[RRLWR_N - 1]);

  for(unsigned int i = 1; i < RRLWR_N; i++) {
    r->coeffs[i] = (uint16_t)(2 * a->coeffs[i] + a->coeffs[i - 1]);
  }
}

static void oracle_ring_mul(ring_element *r,
                            const ring_element *a,
                            const ring_element *b)
{
  memset(r, 0, sizeof(*r));

  for(unsigned int i = 0; i < RRLWR_K; i++) {
    for(unsigned int j = 0; j < RRLWR_K; j++) {
      poly prod;

      oracle_poly_mul(&prod, &a->x[i], &b->x[j]);

      if(i + j < RRLWR_K) {
        oracle_poly_addto(&r->x[i + j], &prod);
      } else {
        poly wrapped;

        oracle_poly_mul_x_plus_2(&wrapped, &prod);
        oracle_poly_addto(&r->x[i + j - RRLWR_K], &wrapped);
      }
    }
  }
}

static int check_u16_array(const char *label,
                           const uint16_t got[RRLWR_N],
                           const uint16_t want[RRLWR_N])
{
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    if(((got[i] ^ want[i]) & (RRLWR_PKE_Q - 1)) != 0) {
      fprintf(stderr,
              "%s mismatch modulo q at coeff %u: got=%u want=%u xor_mod_q=%u\n",
              label,
              i,
              got[i],
              want[i],
              (unsigned int)((got[i] ^ want[i]) & (RRLWR_PKE_Q - 1)));
      return 1;
    }
  }

  return 0;
}

static int check_poly(const char *label, const poly *got, const poly *want)
{
  return check_u16_array(label, got->coeffs, want->coeffs);
}

static int test_basis_products(void)
{
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    for(unsigned int j = 0; j < RRLWR_N; j++) {
      poly a = {0};
      poly b = {0};
      uint16_t got[RRLWR_N];
      uint16_t want[RRLWR_N];

      a.coeffs[i] = (uint16_t)(3u + 257u * i);
      b.coeffs[j] = (uint16_t)(5u + 129u * j);

      poly_mul_toom4_u16(got, &a, &b);
      oracle_poly_mul_acc(want, &a, &b, 1);

      if(check_u16_array("basis poly_mul_toom4_u16", got, want)) {
        fprintf(stderr, "basis indices: i=%u j=%u\n", i, j);
        return 1;
      }
    }
  }

  return 0;
}

static int test_random_poly_products(void)
{
  for(unsigned int t = 0; t < RANDOM_POLY_TESTS; t++) {
    poly a;
    poly b;
    uint16_t got[RRLWR_N];
    uint16_t want[RRLWR_N];

    random_poly(&a);
    random_poly(&b);

    poly_mul_toom4_u16(got, &a, &b);
    oracle_poly_mul_acc(want, &a, &b, 1);

    if(check_u16_array("random poly_mul_toom4_u16", got, want)) {
      fprintf(stderr, "random poly test: t=%u\n", t);
      return 1;
    }

    for(unsigned int i = 0; i < RRLWR_N; i++) {
      got[i] = next_coeff();
      want[i] = got[i];
    }

    poly_macc_toom4_u16(got, &a, &b);
    oracle_poly_mul_acc(want, &a, &b, 0);

    if(check_u16_array("random poly_macc_toom4_u16", got, want)) {
      fprintf(stderr, "random macc test: t=%u\n", t);
      return 1;
    }
  }

  return 0;
}

static int test_random_ring_products(void)
{
  for(unsigned int t = 0; t < RANDOM_RING_TESTS; t++) {
    ring_element a;
    ring_element b;
    ring_element want;
    ring_element_Awin aw;
    poly got[RRLWR_K];
    int ncoeffs_cases[3] = {1, RRLWR_PKE_ELL, RRLWR_K};

    random_ring(&a);
    random_ring(&b);

    oracle_ring_mul(&want, &a, &b);

    for(unsigned int c = 0; c < 3; c++) {
      int ncoeffs = ncoeffs_cases[c];
      int row_min = RRLWR_K - ncoeffs;

      memset(got, 0xa5, sizeof(got));
      ring_mul(got, &a, &b, ncoeffs);

      for(int out = 0; out < ncoeffs; out++) {
        if(check_poly("ring_mul", &got[out], &want.x[row_min + out])) {
          fprintf(stderr, "ring_mul test: t=%u ncoeffs=%d out=%d\n", t, ncoeffs, out);
          return 1;
        }
      }

      ring_to_Awin(&aw, &a);
      memset(got, 0x5a, sizeof(got));
      ring_mul_Awin(got, &aw, &b, ncoeffs);

      for(int out = 0; out < ncoeffs; out++) {
        if(check_poly("ring_mul_Awin", &got[out], &want.x[row_min + out])) {
          fprintf(stderr, "ring_mul_Awin test: t=%u ncoeffs=%d out=%d\n", t, ncoeffs, out);
          return 1;
        }
      }
    }
  }

  return 0;
}

static void oracle_poly_pack(unsigned char *out, const poly *a, int bitlen)
{
  uint32_t mask = (1u << bitlen) - 1u;
  uint32_t bias = (1u << (bitlen - 1)) - 1u;
  unsigned int bitpos = 0;

  memset(out, 0, (size_t)bitlen * RRLWR_N / 8);

  for(unsigned int i = 0; i < RRLWR_N; i++) {
    uint32_t v = (bias - a->coeffs[i]) & mask;

    for(int b = 0; b < bitlen; b++) {
      out[bitpos >> 3] |= (unsigned char)(((v >> b) & 1u) << (bitpos & 7u));
      bitpos++;
    }
  }
}

static void oracle_poly_unpack(poly *r, const unsigned char *in, int bitlen)
{
  uint32_t mask = (1u << bitlen) - 1u;
  uint32_t out_mask = (bitlen == 2) ? (uint32_t)(RRLWR_PKE_Q - 1) : mask;
  uint32_t bias = (1u << (bitlen - 1)) - 1u;
  unsigned int bitpos = 0;

  for(unsigned int i = 0; i < RRLWR_N; i++) {
    uint32_t v = 0;

    for(int b = 0; b < bitlen; b++) {
      v |= (uint32_t)((in[bitpos >> 3] >> (bitpos & 7u)) & 1u) << b;
      bitpos++;
    }

    r->coeffs[i] = (uint16_t)((bias - v) & out_mask);
  }
}

static int test_poly_packing(void)
{
  const int bitlens[] = {1, 2, 3, 11, 13};

  for(unsigned int t = 0; t < RANDOM_PACKING_TESTS; t++) {
    for(unsigned int c = 0; c < sizeof(bitlens) / sizeof(bitlens[0]); c++) {
      int bitlen = bitlens[c];
      size_t nbytes = (size_t)bitlen * RRLWR_N / 8;
      unsigned char got[13 * RRLWR_N / 8];
      unsigned char want[13 * RRLWR_N / 8];
      poly a;
      poly got_unpacked;
      poly want_unpacked;

      for(unsigned int i = 0; i < RRLWR_N; i++) {
        a.coeffs[i] = (uint16_t)(next_coeff() & ((1u << bitlen) - 1u));
      }

      memset(got, 0xa5, sizeof(got));
      memset(want, 0x5a, sizeof(want));

      poly_pack(got, &a, bitlen);
      oracle_poly_pack(want, &a, bitlen);

      if(memcmp(got, want, nbytes) != 0) {
        fprintf(stderr, "poly_pack mismatch: t=%u bitlen=%d\n", t, bitlen);
        return 1;
      }

      poly_unpack(&got_unpacked, got, bitlen);
      oracle_poly_unpack(&want_unpacked, got, bitlen);

      if(check_poly("poly_unpack", &got_unpacked, &want_unpacked)) {
        fprintf(stderr, "poly_unpack test: t=%u bitlen=%d\n", t, bitlen);
        return 1;
      }
    }
  }

  return 0;
}

static int test_message_decoding(void)
{
  const int32_t p = (int32_t)1 << RRLWR_PKE_LOGP;
  const int32_t q = (int32_t)1 << RRLWR_PKE_LOGQ;
  const int32_t t = (int32_t)1 << RRLWR_PKE_LOGT;
  const int32_t correction = q / (2 * p) - p / (2 * t);
  poly cm_poly;
  poly v_poly;
  poly decoded_poly;
  unsigned char packed_cm[RRLWR_PKE_PACKED_POLYT_LEN];
  unsigned char got[RRLWR_PKE_PACKED_POLY1_LEN];

  for(int32_t cm = 0; cm < t; cm++) {
    for(unsigned int i = 0; i < RRLWR_N; i++) {
      cm_poly.coeffs[i] = (uint16_t)cm;
    }
    oracle_poly_pack(packed_cm, &cm_poly, RRLWR_PKE_LOGT);
    poly_unpack(&cm_poly, packed_cm, RRLWR_PKE_LOGT);
    poly_decompress(&cm_poly, RRLWR_PKE_LOGP - RRLWR_PKE_LOGT);

    for(int32_t base = 0; base < p; base += RRLWR_N) {
      for(unsigned int i = 0; i < RRLWR_N; i++) {
        v_poly.coeffs[i] = (uint16_t)(base + (int32_t)i);
      }

      memset(got, 0, sizeof(got));
      poly_subp(&decoded_poly, &v_poly, &cm_poly);
      poly_round_xtoy(&decoded_poly, &decoded_poly, RRLWR_PKE_LOGP, 1);
      poly_pack(got, &decoded_poly, 1);

      for(unsigned int i = 0; i < RRLWR_N; i++) {
        int32_t v = base + (int32_t)i;
        int32_t decoded = v - (p / t) * cm + correction;
        unsigned int want;
        unsigned int actual;

        decoded &= p - 1;
        want = (unsigned int)(((decoded + p / 4) >> (RRLWR_PKE_LOGP - 1)) & 1);
        actual = (unsigned int)((got[i >> 3] >> (i & 7)) & 1);

        if(actual != want) {
          fprintf(stderr,
                  "message decode mismatch: v=%d cm=%d got=%u want=%u correction=%d\n",
                  v,
                  cm,
                  actual,
                  want,
                  correction);
          return 1;
        }
      }
    }
  }

  return 0;
}

int main(void)
{
  rng_state ^= (uint32_t)RRLWR_SECURITY_LEVEL;
  rng_state ^= (uint32_t)RRLWR_K << 16;

  if(test_basis_products()) {
    return 1;
  }

  if(test_random_poly_products()) {
    return 1;
  }

  if(test_random_ring_products()) {
    return 1;
  }

  if(test_poly_packing()) {
    return 1;
  }

  if(test_message_decoding()) {
    return 1;
  }

  printf("Arithmetic correctness tests passed for RRLWR_SECURITY_LEVEL=%d\n",
         RRLWR_SECURITY_LEVEL);
  return 0;
}
