/*
 * bavc.h — Batch All-but-One Vector Commitment (GGM tree) for GALAS.
 *
 * Port of the FAEST reference one-tree BAVC with SHAKE/AES replaced by the
 * xof_* layer (NGCC pseudoXOF underneath). Structure is otherwise faithful to
 * the FAEST reference:
 *   - one GGM tree whose leaves interleave the tau vectors
 *   - prg(seed, iv, alpha) -> 2 seeds, via XOF_DOMAIN_TREE_PRG
 *   - leaf commitment com = PRG(leaf_seed, iv, tweak) -> 2*lambda bits
 *   - tree-root commitments h_i hashed together -> vecCom->h (2*lambda bits)
 *
 * GALAS uses the FAEST-EM-style leaf (no OWF-bound universal-hash key): the
 * leaf seed is the tree leaf itself, and com is a 2*lambda-bit PRG output.
 */
#ifndef GALAS_BAVC_H
#define GALAS_BAVC_H

#include <stdint.h>
#include <stdbool.h>
#include "instances.h"

typedef struct {
    uint8_t* h;    /* root commitment, 2*lambda bits = 2*lambda/8 bytes */
    uint8_t* k;    /* tree node buffer, (2L-1)*lambda bytes */
    uint8_t* com;  /* leaf commitments, L * 2*lambda bits */
    uint8_t* sd;   /* leaf seeds, L * lambda bytes */
} galas_bavc_t;

typedef struct {
    uint8_t* h;    /* reconstructed root commitment, 2*lambda bits */
    uint8_t* s;    /* reconstructed leaf seeds, (L-tau) * lambda bytes */
} galas_bavc_rec_t;

/* BAVC.Commit: build the one-tree commitment from rootKey, produce com[], sd[], h. */
void galas_bavc_commit(const uint8_t* rootKey, const uint8_t* iv,
                       const galas_paramset_t* ps, galas_bavc_t* vc);

/* BAVC.Open: given challenge i_delta[0..tau), produce decommitment decom:
     tau * (2*lambda) bytes of opened leaf comms, then T_open * lambda bytes
     of tree-path seeds. Returns true on success (nh-2*tau+1 <= T_open). */
bool galas_bavc_open(const galas_bavc_t* vc, const uint16_t* i_delta,
                     uint8_t* decom, const galas_paramset_t* ps);

/* BAVC.Reconstruct: verify and rebuild from decom; fills rec->h, rec->s.
   Returns true if the reconstructed root matches (caller compares rec->h to
   vc->h). */
bool galas_bavc_reconstruct(const uint8_t* decom, const uint16_t* i_delta,
                            const uint8_t* iv, const galas_paramset_t* ps,
                            galas_bavc_rec_t* rec);

void galas_bavc_clear(galas_bavc_t* vc);
void galas_bavc_rec_clear(galas_bavc_rec_t* rec);

#endif /* GALAS_BAVC_H */
