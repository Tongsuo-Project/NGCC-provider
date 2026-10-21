/*
 * Copyright (c) 2026 Hang Zhang.
 * State Key Laboratory of Cyberspace Security Defense,
 * Institute of Information Engineering, CAS
 * School of Cyber Security, University of Chinese Academy of Sciences
 */
#ifndef PACKING_H
#define PACKING_H
#include "polyvec.h"

void pack_pk(unsigned char *pk, const unsigned char *seed_A, const polyveck *public_vector);
void pack_sk(unsigned char *sk, const unsigned char *seed_A, const polyveck *public_vector,
             const unsigned char *secret_seed, const unsigned char *tr, const polyvecl *s_0,
             const polyveck *e);
int unpack_sk(unsigned char *seed_A, polyveck *public_vector, unsigned char *secret_seed,
              unsigned char *tr, polyvecl *s_0, polyveck *e, const unsigned char *sk);

int pack_sig(unsigned char *sig, const polyvecm1 *z1, const polyveck *h,
             const unsigned char *challenge);
int unpack_pk(unsigned char *seed_A, polyveck *public_vector, const unsigned char *pk);
int unpack_sig(polyvecm1 *z1, polyveck *h, unsigned char *challenge, const unsigned char *sig);

#endif
