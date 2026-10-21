/* test_sig_roundtrip.c — Sign then Verify round-trip via the NGCC SIG interface.
 * keygen -> sign -> verify should accept; a flipped signature bit should reject.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SIG_AlgorithmInstance.h"

int main(void) {
    unsigned long long pklen, sklen, snlen;
    pklen = sig_get_pk_len_bytes();
    sklen = sig_get_sk_len_bytes();
    snlen = sig_get_sn_len_bytes();
    printf("pk=%llu sk=%llu sn(cap)=%llu\n", pklen, sklen, snlen);

    unsigned char* pk = malloc(pklen);
    unsigned char* sk = malloc(sklen);
    unsigned long long got_pk, got_sk;
    if (sig_keygen(pk, &got_pk, sk, &got_sk) != 0) { printf("FAIL keygen\n"); return 1; }
    printf("keygen ok\n");

    unsigned char* sn = malloc(snlen);
    unsigned long long got_sn;
    unsigned char msg[] = "hello galas ngcc";
    int rc = sig_sign(sk, got_sk, msg, sizeof(msg) - 1, sn, &got_sn);
    printf("sign rc=%d, sn_len=%llu\n", rc, got_sn);
    if (rc != 0) { printf("FAIL sign (rc=%d)\n", rc); return 1; }

    /* verify (should accept) */
    int vr = sig_verify(pk, got_pk, sn, got_sn, msg, sizeof(msg) - 1);
    printf("verify (valid) rc=%d  -> %s\n", vr, vr == 0 ? "ACCEPT" : "REJECT");

    /* tamper: flip a byte in the VOLE correction strings */
    unsigned char* sn2 = malloc(got_sn);
    memcpy(sn2, sn, got_sn);
    sn2[0] ^= 1;
    int vr2 = sig_verify(pk, got_pk, sn2, got_sn, msg, sizeof(msg) - 1);
    printf("verify (tampered c) rc=%d -> %s\n", vr2, vr2 == 0 ? "ACCEPT" : "REJECT");

    /* tamper the QS proof slot.  For GALAS_256S:
       c=(tau-1)*ellhat_bytes, vole_check=lambda/8+2, d=5*lambda/16. */
    unsigned lb = 32;
    unsigned tau = 21;
    unsigned ellhat_bytes = (unsigned)((5 * 256 / 2 + 3 * 256 + 16) / 8);
    unsigned witness_bytes = (5 * 256 / 2) / 8;
    size_t qs_off = (size_t)(tau - 1) * ellhat_bytes + (lb + 2) + witness_bytes;
    unsigned char* sn3 = malloc(got_sn);
    memcpy(sn3, sn, got_sn);
    sn3[qs_off] ^= 1;   /* corrupt qs_response */
    int vr3 = sig_verify(pk, got_pk, sn3, got_sn, msg, sizeof(msg) - 1);
    printf("verify (tampered qs_response) rc=%d -> %s\n", vr3, vr3 == 0 ? "ACCEPT" : "REJECT");

    free(pk); free(sk); free(sn); free(sn2); free(sn3);
    int ok = (vr == 0) && (vr2 != 0) && (vr3 != 0);
    printf("\n%s\n", ok ? "ROUND-TRIP PASS" : "ROUND-TRIP FAIL");
    return ok ? 0 : 1;
}
