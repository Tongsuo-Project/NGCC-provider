#include <stdio.h>
#include <stdlib.h>

#include "scheme_api.h"
#include "rng.h"

int main(void)
{
    unsigned char entropy_input[48];
    unsigned char *pk;
    unsigned char *sk;
    int i;
    int status;
    int t0 = GOPPA_DEGREE / ORDER;

    for (i = 0; i < 48; i++) {
        entropy_input[i] = (unsigned char)i;
    }
    randombytes_init(entropy_input, NULL, 256);

    if ((GOPPA_DEGREE % ORDER) != 0 || (LENGTH % ORDER) != 0) {
        fprintf(stderr, "parameter check failed\n");
        return 2;
    }
    if ((SYSTEMATIC_QC_ROWS % ORDER) != 0 ||
        ((LENGTH - SYSTEMATIC_QC_ROWS) % ORDER) != 0 ||
        SYSTEMATIC_TAIL_ROWS <= 0) {
        printf("%s: public T=no\n", CRYPTO_ALGNAME);
        printf("(m,n,l,t0,t,matgen_rows,w)=(%d,%d,%d,%d,%d,%d,%d)\n",
               EXT_DEGREE, LENGTH, ORDER, t0, GOPPA_DEGREE,
               MATGEN_GOPPA_ROWS, ERROR_WEIGHT);
        printf("qc_rows=%d, qc_rows mod l=%d, pre_tail_public_columns=%d, pre_tail_public_columns mod l=%d, tail_rows=%d\n",
               SYSTEMATIC_QC_ROWS, SYSTEMATIC_QC_ROWS % ORDER,
               LENGTH - SYSTEMATIC_QC_ROWS,
               (LENGTH - SYSTEMATIC_QC_ROWS) % ORDER,
               SYSTEMATIC_TAIL_ROWS);
        return 1;
    }

    pk = calloc((size_t)CRYPTO_PUBLICKEYBYTES, 1);
    sk = calloc((size_t)CRYPTO_SECRETKEYBYTES, 1);
    if (pk == NULL || sk == NULL) {
        free(pk);
        free(sk);
        fprintf(stderr, "allocation failed\n");
        return 2;
    }

    status = crypto_kem_keypair(pk, sk);
    free(pk);
    free(sk);
    if (status != SUCCESS) {
        fprintf(stderr, "keygen failed before producing public T\n");
        return 1;
    }

    printf("%s: generated public T\n", CRYPTO_ALGNAME);
    printf("(m,n,l,t0,t,matgen_rows,w)=(%d,%d,%d,%d,%d,%d,%d)\n",
           EXT_DEGREE, LENGTH, ORDER, t0, GOPPA_DEGREE,
           MATGEN_GOPPA_ROWS, ERROR_WEIGHT);
    printf("compressed public key bytes=%d\n", CRYPTO_PUBLICKEYBYTES);
    printf("codimension=%d dimension=%d\n", CODIMENSION, DIMENSION);
    printf("qc_rows=%d tail_rows=%d public_t_columns=%d\n",
           SYSTEMATIC_QC_ROWS, SYSTEMATIC_TAIL_ROWS, PUBLIC_T_COLUMNS);
    printf("psi_t1_rows=%d psi_t1_bits=%d t2_bits=%d publickey_bits=%d\n",
           PSI_T1_ROWS, PSI_T1_BITS, T2_BITS, PUBLICKEY_BITS);
    return 0;
}
