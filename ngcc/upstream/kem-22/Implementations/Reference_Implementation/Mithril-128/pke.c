#include "pke.h"
#include "packing.h"

void poly_subp(poly *r, const poly *f, const poly *g) {
  const int32_t correction =
    ((int32_t)1 << (RRLWR_PKE_LOGQ-(RRLWR_PKE_LOGP+1))) - // q/(2*p)
    ((int32_t)1 << (RRLWR_PKE_LOGP-(RRLWR_PKE_LOGT+1)));  // p/(2*t)

  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] = (f->coeffs[i] - g->coeffs[i] + correction) & (RRLWR_PKE_P-1); // Reduce mod p
  }
}

void poly_add_msg(poly *r, const unsigned char msg[RRLWR_PKE_MESSAGE_LEN]) {
  int32_t mj;

  for(unsigned int i = 0; i < RRLWR_PKE_ELL; i++) {
    for(unsigned int j = 0; j < RRLWR_N; j++) {
      mj = (msg[i*RRLWR_N/8 + (j >> 3)] >> (j & 0x7)) & 1;                 // Select coefficient j of polynomial i
      r[i].coeffs[j] += (int32_t)1 << (RRLWR_PKE_LOGQ-(RRLWR_PKE_LOGP+1)); // Add q/(2*p)
      r[i].coeffs[j] += -mj & ((int32_t)1 << (RRLWR_PKE_LOGP-1));          // Add p/2 if message bit is 1, otherwise not
      r[i].coeffs[j] &= (RRLWR_PKE_P-1);                                   // Reduce mod p
    }
  }
}

uint8_t ct_cmp(const unsigned char *c1, const unsigned char *c2) {
  uint8_t r = 0;
  for(unsigned int i = 0; i < RRLWR_KEM_CT_LEN; i++) {
    r |= (c1[i] ^ c2[i]); // Only non-zero if c1 != c2
  }

  return -((-(uint32_t)r) >> 31);
}

int pke_keygen(unsigned char pk[RRLWR_PKE_PK_LEN], unsigned char sk[RRLWR_PKE_SK_LEN],
	               const unsigned char seedA[RRLWR_PKE_SEED_A_LEN], const unsigned char seedS[RRLWR_SEED_S_LEN]) {

  ring_element_Awin a;
  ring_element s, b;

  // Generate a with coefficients in [-q/2+1, q/2]
  ring_uniform_Awin(&a, RRLWR_PKE_LOGQ, seedA, RRLWR_PKE_SEED_A_LEN);

  // Generate s with coefficients in [-2, 1] and pack
  ring_uniform(&s, RRLWR_PKE_LOG_ETA+1, seedS, RRLWR_SEED_S_LEN);
  ring_pack(sk, &s, RRLWR_PKE_LOG_ETA+1);

  // Compute b = round(p/q*A*s) directly from the lazy R_q accumulator.
  ring_mul_Awin_round_p(b.x, &a, &s, RRLWR_K);

  // Pack the public key
  for (unsigned int i = 0; i < RRLWR_PKE_SEED_A_LEN; i++) {
    pk[i] = seedA[i];
  }
  ring_pack(pk + RRLWR_PKE_SEED_A_LEN, &b, RRLWR_PKE_LOGP);

  return 0;
}

int pke_encrypt(unsigned char ct[RRLWR_PKE_CT_LEN], const unsigned char pk[RRLWR_PKE_PK_LEN],
                const unsigned char m[RRLWR_PKE_MESSAGE_LEN], const unsigned char seedSp[RRLWR_SEED_S_LEN]) {

  ring_element_Awin a;
  ring_element_Awin bw;
  ring_element sp, bp;
  const unsigned char *seedA = &pk[0];

  // Generate a with coefficients in [-q/2+1, q/2]
  ring_uniform_Awin(&a, RRLWR_PKE_LOGQ, seedA, RRLWR_PKE_SEED_A_LEN);

  // Generate s_prime with coefficients in [-2, 1]
  ring_uniform(&sp, RRLWR_PKE_LOG_ETA+1, seedSp, RRLWR_SEED_S_LEN);

  // Compute b_prime = round(p/q*A*s_prime) directly from the lazy R_q accumulator.
  ring_mul_Awin_round_p(bp.x, &a, &sp, RRLWR_K);

  // Compute v_prime = b*s_prime
  ring_unpack_Awin(&bw, pk + RRLWR_PKE_SEED_A_LEN, RRLWR_PKE_LOGP);
  ring_mul_Awin_add_msg_pack_t(ct, &bw, &sp, m);

  // Return ct = (cm = vp, b) and sk = s
  ring_pack(ct + RRLWR_PKE_ELL*RRLWR_PKE_PACKED_POLYT_LEN, &bp, RRLWR_PKE_LOGP);

  return 0;
}

int pke_decrypt(unsigned char m[RRLWR_PKE_MESSAGE_LEN], 
                const unsigned char ct[RRLWR_PKE_CT_LEN], const unsigned char sk[RRLWR_PKE_SK_LEN]) {
  ring_element s;
  ring_element_Awin bpw;
  poly v[RRLWR_PKE_ELL];
  poly cm[RRLWR_PKE_ELL];

  // Unpack s, b and cm, then decompress cm from R_t to R_p.
  ring_unpack(&s, sk, RRLWR_PKE_LOG_ETA+1);
  ring_unpack_Awin(&bpw, ct + RRLWR_PKE_ELL*RRLWR_PKE_PACKED_POLYT_LEN, RRLWR_PKE_LOGP);
  for(unsigned int i = 0; i < RRLWR_PKE_ELL; i++) {
    poly_unpack(&cm[i], ct + i*RRLWR_PKE_PACKED_POLYT_LEN, RRLWR_PKE_LOGT);
    poly_decompress(&cm[i], RRLWR_PKE_LOGP - RRLWR_PKE_LOGT);
  }

  // Compute v = b_prime*s.
  ring_mul_Awin(v, &bpw, &s, RRLWR_PKE_ELL);

  // Subtract cm, apply the decryption correction, round, and pack the message.
  for(unsigned int i = 0; i < RRLWR_PKE_ELL; i++) {
    poly_subp(&v[i], &v[i], &cm[i]);
    poly_round_xtoy(&v[i], &v[i], RRLWR_PKE_LOGP, 1);
    poly_pack(m + i*RRLWR_PKE_PACKED_POLY1_LEN, &v[i], 1);
  }

  return 0;
}
