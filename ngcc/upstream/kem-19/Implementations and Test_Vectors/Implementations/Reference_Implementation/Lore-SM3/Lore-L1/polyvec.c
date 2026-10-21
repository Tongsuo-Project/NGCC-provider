#include "polyvec.h"
#include "poly.h"
#include "symmetric.h"
#include <string.h> // for memset

/* Apply NTT to all elements of a vector of polynomials */
void polyvec_ntt(polyvec *v) {
    for (int i = 0; i < LORE_K; i++) {
        poly_ntt(&v->vec[i]);
    }
}

/* Apply inverse NTT to all elements of a vector of polynomials */
void polyvec_invntt_tomont(polyvec *v) {
    for (int i = 0; i < LORE_K; i++) {
        poly_invntt_tomont(&v->vec[i]);
    }
}

/* Pointwise multiply elements of a and b and accumulate into r */
void polyvec_pointwise_acc_montgomery(poly *r, const polyvec *a, const polyvec *b) {
    poly t;
    poly_pointwise_montgomery(r, &a->vec[0], &b->vec[0]);
    for (int i = 1; i < LORE_K; i++) {
        poly_pointwise_montgomery(&t, &a->vec[i], &b->vec[i]);
        poly_add(r, r, &t);
    }
}

/* Add vectors of polynomials */
void polyvec_add(polyvec *r, const polyvec *a, const polyvec *b) {
    for (int i = 0; i < LORE_K; i++) {
        poly_add(&r->vec[i], &a->vec[i], &b->vec[i]);
    }
}

/* Add two vectors of CRT polynomials */
void poly_crt_vec_add(poly_crt_vec *r, const poly_crt_vec *a, const poly_crt_vec *b) {
    for(int i=0; i<LORE_K; ++i)
        poly_crt_add(&r->vec[i], &a->vec[i], &b->vec[i]);
}

/* Apply NTT to the q-part of a vector of CRT polynomials */
void poly_crt_vec_ntt(poly_crt_vec *v) {
    for (int i = 0; i < LORE_K; ++i)
        poly_ntt(&v->vec[i].q_poly);
}

/* Apply inverse NTT to the q-part of a vector of CRT polynomials */
void poly_crt_vec_invntt_tomont(poly_crt_vec *v) {
    for (int i = 0; i < LORE_K; ++i)
        poly_invntt_tomont(&v->vec[i].q_poly);
}

/* (Dense * Dense) */
void poly_crt_vec_pointwise_acc_montgomery(poly_crt *r, const poly_crt_vec *a_row_dense, const poly_crt_vec *b_dense) {
    poly_crt t;

    /* q-part (NTT domain) */
    poly_pointwise_montgomery(&r->q_poly, &a_row_dense->vec[0].q_poly, &b_dense->vec[0].q_poly);
    for (int i = 1; i < LORE_K; i++) {
        poly_pointwise_montgomery(&t.q_poly, &a_row_dense->vec[i].q_poly, &b_dense->vec[i].q_poly);
        poly_add(&r->q_poly, &r->q_poly, &t.q_poly);
    }

    poly t_prod;
    poly_mul_modt(&r->t_poly, &a_row_dense->vec[0].t_poly, &b_dense->vec[0].t_poly);

    for (int i = 1; i < LORE_K; i++) {
        poly_mul_modt(&t_prod, &a_row_dense->vec[i].t_poly, &b_dense->vec[i].t_poly);
        poly_add_modt(&r->t_poly, &r->t_poly, &t_prod);
    }
}

/* (Dense * Sparse) */
void poly_crt_vec_pointwise_acc_montgomery_sparse(poly_crt *r, const poly_crt_vec *a_row_dense, const poly_crt_vec *b_crt_with_ntt_q, const poly_sparse *b_t_poly_sparse_vec) {
    poly_crt t_q_only;

    /* q-part (in NTT domain) */
    poly_pointwise_montgomery(&r->q_poly, &a_row_dense->vec[0].q_poly, &b_crt_with_ntt_q->vec[0].q_poly);
    for (int i = 1; i < LORE_K; i++) {
        poly_pointwise_montgomery(&t_q_only.q_poly, &a_row_dense->vec[i].q_poly, &b_crt_with_ntt_q->vec[i].q_poly);
        poly_add(&r->q_poly, &r->q_poly, &t_q_only.q_poly);
    }

#if LORE_LEVEL == 1
    poly_sparse_mul_modt(&r->t_poly, &a_row_dense->vec[0].t_poly, &b_t_poly_sparse_vec[0]);
    for (int i = 1; i < LORE_K; i++) {
        poly t_temp_prod;
        poly_sparse_mul_modt(&t_temp_prod, &a_row_dense->vec[i].t_poly, &b_t_poly_sparse_vec[i]);
        poly_add_modt(&r->t_poly, &r->t_poly, &t_temp_prod);
    }
#else
    poly b_t_dense_temp;

    poly_from_sparse(&b_t_dense_temp, &b_t_poly_sparse_vec[0]);
    poly_mul_modt(&r->t_poly, &a_row_dense->vec[0].t_poly, &b_t_dense_temp);

    for (int i = 1; i < LORE_K; i++) {
        poly t_temp_prod_local;
        poly_from_sparse(&b_t_dense_temp, &b_t_poly_sparse_vec[i]);
        poly_mul_modt(&t_temp_prod_local, &a_row_dense->vec[i].t_poly, &b_t_dense_temp);
        poly_add_modt(&r->t_poly, &r->t_poly, &t_temp_prod_local);
    }
#endif
}

/* Generates a matrix of CRT polynomials in standard domain. */
void gen_matrix_std(poly_crt_vec a[LORE_K], const unsigned char *seed)
{
    gen_matrix_ntt(a, seed, 0);
}

/* Generates a matrix of CRT polynomials with q-parts in NTT domain. */
void gen_matrix_ntt(poly_crt_vec a[LORE_K], const unsigned char *seed, int transposed)
{
  unsigned int i, j;

  // Each CRT component has its own domain-separated XOF stream. The refill
  // loops below handle the full n=1024 output even after rejections.
  const unsigned int NBLOCKS = 5;
  uint8_t buf[NBLOCKS * XOF_BLOCKBYTES];

  for (i = 0; i < LORE_K; i++) {
    for (j = 0; j < LORE_K; j++) {
      xof_state state_q, state_t;
      // Use disjoint input domains for the q and t CRT components.
      if (transposed) {
          xof_absorb(&state_q, seed, (uint8_t)j, (uint8_t)i);
          xof_absorb(&state_t, seed, (uint8_t)(j + LORE_K), (uint8_t)i);
      } else {
          xof_absorb(&state_q, seed, (uint8_t)i, (uint8_t)j);
          xof_absorb(&state_t, seed, (uint8_t)(i + LORE_K), (uint8_t)j);
      }
      xof_squeezeblocks(buf, NBLOCKS, &state_q);

      // -- Generate q-part --
      unsigned int ctr_q = 0;
      ctr_q = rej_uniform_q(a[i].vec[j].q_poly.coeffs, LORE_N, buf, NBLOCKS * XOF_BLOCKBYTES);
      // In the rare case the buffer is not enough, squeeze another block
      while(ctr_q < LORE_N) {
        xof_squeezeblocks(buf, 1, &state_q);
        ctr_q += rej_uniform_q(a[i].vec[j].q_poly.coeffs + ctr_q, LORE_N - ctr_q, buf, XOF_BLOCKBYTES);
      }

      // -- Generate t-part --
      unsigned int ctr_t = 0;
      xof_squeezeblocks(buf, NBLOCKS, &state_t);
      ctr_t = rej_uniform_t(a[i].vec[j].t_poly.coeffs, LORE_N, buf, NBLOCKS * XOF_BLOCKBYTES);
      // In the rare case the buffer is not enough, squeeze another block
      while(ctr_t < LORE_N) {
        xof_squeezeblocks(buf, 1, &state_t);
        ctr_t += rej_uniform_t(a[i].vec[j].t_poly.coeffs + ctr_t, LORE_N - ctr_t, buf, XOF_BLOCKBYTES);
      }
    }
  }
}
