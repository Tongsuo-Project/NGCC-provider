#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "goppa.h"
#include "scheme_api.h"
#include "rng.h"

int main(void)
{
    unsigned char entropy_input[48];
    unsigned char pk[CRYPTO_PUBLICKEYBYTES];
    unsigned char pk2[CRYPTO_PUBLICKEYBYTES];
    unsigned char sk[CRYPTO_SECRETKEYBYTES];
    unsigned char *p;
    gfelt_t *Ltmp;
    poly_t g;
    gf_t eta;
    int i;
    int rc;

    for (i = 0; i < 48; i++) {
        entropy_input[i] = (unsigned char)i;
    }
    randombytes_init(entropy_input, NULL, 256);

    if (crypto_kem_keypair(pk, sk) != SUCCESS) {
        puts("keypair=fail");
        return 1;
    }

    gf_init(EXT_DEGREE);
    p = sk;
    Ltmp = malloc((size_t)LENGTH * sizeof(*Ltmp));
    if (Ltmp == NULL) {
        return 2;
    }
    memcpy(Ltmp, p, (size_t)LENGTH * sizeof(*Ltmp));
    p += LENGTH * sizeof(gfelt_t);

    g = poly_alloc_from_string(GOPPA_DEGREE, p);
    if (g == NULL) {
        free(Ltmp);
        return 2;
    }
    poly_set_deg(g, GOPPA_DEGREE);
    p += (GOPPA_DEGREE + 1) * sizeof(gfelt_t);

    gf_set(eta, (gfelt_t *)p);
    memset(pk2, 0, sizeof(pk2));
    rc = goppa_keygen(Ltmp, g, LENGTH, ORDER, GOPPA_DEGREE, eta, pk2, NULL);

    printf("recompute_rc=%d\n", rc);
    printf("pk_equal=%s\n", memcmp(pk, pk2, sizeof(pk)) == 0 ? "yes" : "no");

    free(g);
    free(Ltmp);
    return rc == CODIMENSION && memcmp(pk, pk2, sizeof(pk)) == 0 ? 0 : 1;
}
