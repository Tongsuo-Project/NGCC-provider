#ifndef INDCPA_H
#define INDCPA_H

#include <stdint.h>
#include "params.h"
#include "polyvec.h" 

#define indcpa_keypair_derand LORE_NAMESPACE(indcpa_keypair_derand)
void indcpa_keypair_derand(unsigned char *pk, unsigned char *sk, const unsigned char *coins);

#define indcpa_keypair LORE_NAMESPACE(indcpa_keypair)
void indcpa_keypair(unsigned char *pk, unsigned char *sk);

#define indcpa_enc LORE_NAMESPACE(indcpa_enc)
void indcpa_enc(unsigned char *c,
                const unsigned char *m,
                const unsigned char *pk,
                const unsigned char *coins);

#define indcpa_dec LORE_NAMESPACE(indcpa_dec)
/* Returns -1 for an invalid sparse-secret-key encoding, otherwise 0.
 * Callers must supply full-sized buffers as specified by params.h. */
int indcpa_dec(unsigned char *m,
                const unsigned char *c,
                const unsigned char *sk);

#endif
