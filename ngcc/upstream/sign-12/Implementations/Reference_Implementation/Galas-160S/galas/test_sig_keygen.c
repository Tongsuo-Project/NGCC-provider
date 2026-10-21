/* test_sig_keygen.c — verify GALAS KeyGen produces a valid (pk, sk).
 * Runs sig_keygen, then checks pk = (x, y) with y = Gala_k(x) by recomputing
 * the OWF from sk. This is the end-to-end checkable part of the SIG interface.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SIG_AlgorithmInstance.h"
#include "../galas/gf2n.h"
#include "../galas/owf.h"
#include "../galas/bf.h"

int main(void) {
    unsigned long long pklen, sklen, snlen;
    pklen = sig_get_pk_len_bytes();
    sklen = sig_get_sk_len_bytes();
    snlen = sig_get_sn_len_bytes();
    printf("pk=%llu sk=%llu sn(est)=%llu\n", pklen, sklen, snlen);

    unsigned char* pk = malloc(pklen);
    unsigned char* sk = malloc(sklen);
    if (sig_keygen(pk, &pklen, sk, &sklen) != 0) {
        printf("FAIL: sig_keygen returned nonzero\n"); return 1;
    }
    printf("keygen ok: pklen=%llu sklen=%llu\n", pklen, sklen);

    /* SK layout is x || k.  KeyGen constraint: k[0] & 0x03 must be 0x03. */
    unsigned lambda = 256;   /* GALAS_256S */
    unsigned lb = lambda / 8;
    const unsigned char* sk_x = sk;
    const unsigned char* sk_k = sk + lb;
    if ((sk_k[0] & 0x03) != 0x03) {
        printf("FAIL: k[0]&0x03 != 0x03 (KeyGen constraint violated)\n"); return 1;
    }
    printf("ok: k[0]&0x03 == 0x03 (KeyGen k[0]=k[1]=1 constraint satisfied)\n");

    /* verify y = Gala_k(x): recompute OWF from sk.k and pk.x */
    const galas_owf_params* P = galas_owf_params_for(lambda);
    const gf_ctx* fc = bf_ctx_256();
    unsigned char y_recomp[64];
    if (memcmp(sk_x, pk, lb) != 0) {
        printf("FAIL: sk.x != pk.x\n"); return 1;
    }
    int rc = galas_owf_eval(P, (gf_ctx*)fc, y_recomp, pk /*x*/, sk_k /*k*/);
    if (rc != 0) { printf("FAIL: OWF rejected on keygen output\n"); return 1; }
    if (memcmp(y_recomp, pk + lb, lb) != 0) {
        printf("FAIL: recomputed y != pk.y\n"); return 1;
    }
    printf("ok: y = Gala_k(x) matches (keygen produces a valid Gala keypair)\n");

    free(pk); free(sk);
    return 0;
}
