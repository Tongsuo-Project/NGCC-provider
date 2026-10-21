#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "drng.h"
#include "parameters.h"
#include "poly.h"
#include "ring.h"
#include "pke.h"

#define RNG_SEED_LENGTH 32
#define NUMBER_OF_TESTS 100

DRNG_ctx drng_algorithm;

static int32_t generate_random_int32(DRNG_ctx *drng) {
  unsigned char x[4];
  get_random_number(drng, x, 32);
  return x[0] | (x[1] << 8) | (x[2] << 16) | (x[3] << 24);
}

static uint16_t reduce_mod_q(int32_t x)
{
  return (uint16_t)(x & (RRLWR_PKE_Q - 1));
}

static void random_poly_mod_q(poly *r)
{
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] = reduce_mod_q(generate_random_int32(&drng_algorithm));
  }
}

static int poly_equal_mod_q(const poly *a, const poly *b)
{
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    if(((a->coeffs[i] ^ b->coeffs[i]) & (RRLWR_PKE_Q - 1)) != 0) {
      return 0;
    }
  }

  return 1;
}

static int ring_equal_mod_q(const ring_element *a, const ring_element *b)
{
  for(unsigned int i = 0; i < RRLWR_K; i++) {
    if(!poly_equal_mod_q(&a->x[i], &b->x[i])) {
      return 0;
    }
  }

  return 1;
}

static void ref_poly_mul_schoolbook(poly *h, const poly *f, const poly *g)
{
  uint16_t acc[RRLWR_N] = {0};

  for(unsigned int i = 0; i < RRLWR_N; i++) {
    for(unsigned int j = 0; j < RRLWR_N; j++) {
      uint16_t t = (uint16_t)((uint32_t)f->coeffs[i] * g->coeffs[j]);
      unsigned int d = i + j;

      if(d < RRLWR_N) {
        acc[d] = (uint16_t)(acc[d] + t);
      } else {
        acc[d - RRLWR_N] = (uint16_t)(acc[d - RRLWR_N] - t);
      }
    }
  }

  for(unsigned int i = 0; i < RRLWR_N; i++) {
    h->coeffs[i] = acc[i];
  }
}

static void poly_mul_x_plus_2(poly *r, const poly *f)
{
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    uint16_t xterm = (i == 0) ? (uint16_t)(-f->coeffs[RRLWR_N - 1]) : f->coeffs[i - 1];
    r->coeffs[i] = (uint16_t)(2 * f->coeffs[i] + xterm);
  }
}

static void poly_addto(poly *r, const poly *f)
{
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] = (uint16_t)(r->coeffs[i] + f->coeffs[i]);
  }
}

static void ref_ring_mul(ring_element *h, const ring_element *a, const ring_element *s)
{
  poly t, wrapped;

  memset(h, 0, sizeof(*h));

  for(unsigned int i = 0; i < RRLWR_K; i++) {
    for(unsigned int j = 0; j < RRLWR_K; j++) {
      ref_poly_mul_schoolbook(&t, &a->x[i], &s->x[j]);

      if(i + j < RRLWR_K) {
        poly_addto(&h->x[i + j], &t);
      } else {
        poly_mul_x_plus_2(&wrapped, &t);
        poly_addto(&h->x[i + j - RRLWR_K], &wrapped);
      }
    }
  }
}

static int test_poly_mul_toom4(void)
{
  poly f, g, hp, hs;

  random_poly_mod_q(&f);
  random_poly_mod_q(&g);

  poly_mul_toom4(&hp, &f, &g);
  ref_poly_mul_schoolbook(&hs, &f, &g);

  assert(poly_equal_mod_q(&hp, &hs));

  return 0;
}

static int test_ring_mul(void)
{
  ring_element r, h, a, s;
  ring_element_Awin aw;

  for(unsigned int i = 0; i < RRLWR_K; i++) {
    random_poly_mod_q(&a.x[i]);
    random_poly_mod_q(&s.x[i]);
  }

  ring_mul(r.x, &a, &s, RRLWR_K);
  ref_ring_mul(&h, &a, &s);

  assert(ring_equal_mod_q(&r, &h));

  ring_to_Awin(&aw, &a);
  ring_mul_Awin(r.x, &aw, &s, RRLWR_K);

  assert(ring_equal_mod_q(&r, &h));

  return 0;
}

static int test_packing(void) {
  unsigned char buffer2[RRLWR_K*RRLWR_PACKED_POLY2_LEN];
  unsigned char buffert[RRLWR_K*RRLWR_PKE_PACKED_POLYT_LEN];
  unsigned char bufferp[RRLWR_K*RRLWR_PKE_PACKED_POLYP_LEN];
  unsigned char bufferq[RRLWR_K*RRLWR_PKE_PACKED_POLYQ_LEN];
  ring_element r, rc1, rc2;
  int32_t q;

  q = (int32_t)1 << 2;
  for(unsigned int j = 0; j < RRLWR_K; j++) {
    for(unsigned int k = 0; k < RRLWR_N; k++) {
      r.x[j].coeffs[k] = (uint16_t)((1 - (((generate_random_int32(&drng_algorithm) % q) + q) % q)) &
                                    (RRLWR_PKE_Q - 1));
      rc1.x[j].coeffs[k] = r.x[j].coeffs[k];
    }
  }

  ring_pack(buffer2, &r, RRLWR_PKE_LOG_ETA+1);
  ring_unpack(&rc2, buffer2, RRLWR_PKE_LOG_ETA+1);
  assert(!memcmp(&rc1, &rc2, sizeof(ring_element)));

  q = (int32_t)1 << RRLWR_PKE_LOGQ;
  for(unsigned int j = 0; j < RRLWR_K; j++) {
    for(unsigned int k = 0; k < RRLWR_N; k++) {
      r.x[j].coeffs[k] = (uint16_t)(((q/2)-1-(((generate_random_int32(&drng_algorithm) % q) + q) % q)) & (q - 1));
      rc1.x[j].coeffs[k] = r.x[j].coeffs[k];
    }
  }

  ring_pack(bufferq, &r, RRLWR_PKE_LOGQ);
  ring_unpack(&rc2, bufferq, RRLWR_PKE_LOGQ);
  assert(!memcmp(&rc1, &rc2, sizeof(ring_element)));

  q = (int32_t)1 << RRLWR_PKE_LOGP;
  for(unsigned int j = 0; j < RRLWR_K; j++) {
    for(unsigned int k = 0; k < RRLWR_N; k++) {
      r.x[j].coeffs[k] = (uint16_t)(((q/2)-1-(((generate_random_int32(&drng_algorithm) % q) + q) % q)) & (q - 1));
      rc1.x[j].coeffs[k] = r.x[j].coeffs[k];
    }
  }

  ring_pack(bufferp, &r, RRLWR_PKE_LOGP);
  ring_unpack(&rc2, bufferp, RRLWR_PKE_LOGP);
  assert(!memcmp(&rc1, &rc2, sizeof(ring_element)));

  q = (int32_t)1 << RRLWR_PKE_LOGT;
  for(unsigned int j = 0; j < RRLWR_K; j++) {
    for(unsigned int k = 0; k < RRLWR_N; k++) {
      r.x[j].coeffs[k] = (uint16_t)(((q/2)-1-(((generate_random_int32(&drng_algorithm) % q) + q) % q)) & (q - 1));
      rc1.x[j].coeffs[k] = r.x[j].coeffs[k];
    }
  }

  ring_pack(buffert, &r, RRLWR_PKE_LOGT);
  ring_unpack(&rc2, buffert, RRLWR_PKE_LOGT);
  assert(!memcmp(&rc1, &rc2, sizeof(ring_element)));

  return 0;
}

static int test_pke(void) {
  unsigned char seedA[RRLWR_PKE_SEED_A_LEN];
  unsigned char seedS[RRLWR_SEED_S_LEN];
  unsigned char seedSp[RRLWR_SEED_S_LEN];
  unsigned char sk[RRLWR_PKE_SK_LEN];
  unsigned char pk[RRLWR_PKE_PK_LEN];
  unsigned char ct[RRLWR_PKE_CT_LEN];
  unsigned char m[RRLWR_PKE_MESSAGE_LEN];
  unsigned char mp[RRLWR_PKE_MESSAGE_LEN];

  get_random_number(&drng_algorithm, seedA,  8*RRLWR_PKE_SEED_A_LEN);
  get_random_number(&drng_algorithm, seedS,  8*RRLWR_SEED_S_LEN);
  get_random_number(&drng_algorithm, seedSp, 8*RRLWR_SEED_S_LEN);
  get_random_number(&drng_algorithm, m,      8*RRLWR_PKE_MESSAGE_LEN);

  pke_keygen(pk, sk, seedA, seedS);
  pke_encrypt(ct, pk, m, seedSp);
  pke_decrypt(mp, ct, sk);

  assert(!memcmp(m, mp, RRLWR_PKE_MESSAGE_LEN));

  return 0;
}

int main(void) {
  const unsigned char seed[RNG_SEED_LENGTH] = {0};
  init_random_number(&drng_algorithm, seed, RNG_SEED_LENGTH);

  for(unsigned int i = 0; i < NUMBER_OF_TESTS; i++) {
    test_poly_mul_toom4();
    test_ring_mul();
    test_packing();
    test_pke();
  }

  printf("Success!\n");
}
