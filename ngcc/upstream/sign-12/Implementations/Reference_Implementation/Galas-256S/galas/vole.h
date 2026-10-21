/*
 * vole.h — subfield-VOLE commit/reconstruct for GALAS.
 *
 * Port of RefCodes/ref-FAEST/faest_128f/vole.c, with prg routed through the
 * xof_* layer (NGCC pseudoXOF). The BAVC produces tau trees of leaf seeds;
 * ConvertToVole turns each tree's seeds into one column of the subfield VOLE:
 *   u[i]  (the masked-witness bit-vector for instance i)
 *   v[i][d] for d in [0, depth_i)  (the VOLE tags at each tree level)
 * The concatenation over i gives the full (u, v) VOLE correlation of length
 * ell_hat bits per row, with lambda rows (one per tree level + padding).
 */
#ifndef GALAS_VOLE_H
#define GALAS_VOLE_H

#include <stdint.h>
#include <stdbool.h>
#include "bavc.h"
#include "instances.h"

/* Convert one tree's leaf seeds (sd, Ni entries of lambda bytes each) into a
   VOLE column. Produces `depth` v-rows (each ellhat_bytes) and, if u != NULL
   and sd0_bot==false, one u-row. Returns depth (= number of v rows produced).
   Mirrors FAEST ConvertToVole. */
int galas_convert_to_vole(const uint8_t* iv, const uint8_t* sd, int sd0_bot,
                          unsigned i, unsigned ellhat_bytes,
                          uint8_t* u, uint8_t* v /* depth*ellhat_bytes */,
                          const galas_paramset_t* ps);

/* VOLE.Commit: build the BAVC, then run ConvertToVole per tree.
   Outputs:
     vc            (the bavc, already committed)
     c[(tau-1)*ellhat_bytes]   correction strings
     u[ellhat_bytes]           u = u_0
     v[lambda][ellhat_bytes]   the lambda VOLE rows (caller allocates each) */
void galas_vole_commit(const uint8_t* rootKey, const uint8_t* iv, unsigned ellhat,
                       const galas_paramset_t* ps, galas_bavc_t* vc,
                       uint8_t* c, uint8_t* u, uint8_t** v);

/* VOLE.Reconstruct: rebuild the verifier's q[lambda][ellhat_bytes] view from
   the decommitment + corrections + the challenged point delta.
   Returns true on success. */
bool galas_vole_reconstruct(uint8_t* com, uint8_t** q, const uint8_t* iv,
                            const uint16_t* i_delta, const uint8_t* decom,
                            const uint8_t* c, unsigned ellhat,
                            const galas_paramset_t* ps);

#endif /* GALAS_VOLE_H */
