/* test_all_instances.c — keygen+sign+verify for ALL 8 GALAS instances.
 * Compiles each instance via -DGALAS_INSTANCE=... and runs roundtrip. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SIG_AlgorithmInstance.h"

int main(void) {
    unsigned long long pklen = sig_get_pk_len_bytes();
    unsigned long long sklen = sig_get_sk_len_bytes();
    unsigned long long snlen = sig_get_sn_len_bytes();
    printf("instance: pk=%llu sk=%llu sn(cap)=%llu\n", pklen, sklen, snlen);

    unsigned char* pk = malloc(pklen);
    unsigned char* sk = malloc(sklen);
    unsigned char* sn = malloc(snlen);
    unsigned long long gpk, gsk, gsn;

    /* keygen */
    if (sig_keygen(pk, &gpk, sk, &gsk) != 0) { printf("FAIL keygen\n"); return 1; }
    printf("keygen ok (k[0]&0x03=%02x)\n", sk[sklen / 2] & 0x03);

    /* sign */
    unsigned char msg[] = "test all instances";
    int rc = sig_sign(sk, gsk, msg, sizeof(msg)-1, sn, &gsn);
    if (rc != 0) { printf("FAIL sign rc=%d\n", rc); return 1; }
    printf("sign ok sn_len=%llu\n", gsn);

    /* verify valid */
    int vr = sig_verify(pk, gpk, sn, gsn, msg, sizeof(msg)-1);
    printf("verify(valid)=%s\n", vr==0?"ACCEPT":"REJECT");

    /* verify tampered */
    sn[0] ^= 1;
    int vr2 = sig_verify(pk, gpk, sn, gsn, msg, sizeof(msg)-1);
    printf("verify(tampered)=%s\n", vr2!=0?"REJECT":"ACCEPT(BUG)");

    int ok = (vr==0) && (vr2!=0);
    printf("%s\n", ok ? "INSTANCE PASS" : "INSTANCE FAIL");
    free(pk); free(sk); free(sn);
    return ok ? 0 : 1;
}
