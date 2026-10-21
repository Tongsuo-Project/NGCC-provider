#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scheme_api.h"
#include "seeded_keygen.h"
#include "rng.h"

int main(void)
{
    unsigned char entropy_input[48];
    unsigned char seed[KEYGEN_SEED_BYTES];
    unsigned char *pk = NULL;
    goppa_t gamma;
    gf_t eta;
    int i;
    int rank;
    int t0 = GOPPA_DEGREE / ORDER;
    int t;
    const char *t0_text = getenv("LOCALLY_QUASI_CYCLIC_TWISTED_MCELIECE_AUDIT_T0");

    for (i = 0; i < 48; i++) {
        entropy_input[i] = (unsigned char)i;
    }
    randombytes_init(entropy_input, NULL, 256);
    randombytes(seed, sizeof(seed));

    gf_init(EXT_DEGREE);
    if (!goppa_fixed_eta(ORDER, eta)) {
        fprintf(stderr, "eta generation failed\n");
        return 2;
    }
    if (t0_text != NULL && t0_text[0] != '\0') {
        char *end = NULL;
        long requested = strtol(t0_text, &end, 10);

        if (end != t0_text && *end == '\0' && requested > 0 &&
            requested < 1000) {
            t0 = (int)requested;
        }
    }
    t = ORDER * t0;

    gamma = scheme_keygen_seeded(LENGTH, ORDER, EXT_DEGREE, t,
                               eta, seed, sizeof(seed), NULL, NULL, NULL, NULL);
    if (gamma == NULL) {
        fprintf(stderr, "seeded Gamma generation failed\n");
        return 1;
    }

    rank = goppa_matgen_audit(gamma->L, gamma->g, LENGTH, ORDER, t, eta,
                              stdout);
    free_goppa(gamma);
    free(pk);
    return rank == SYSTEMATIC_QC_ROWS ? 0 : 1;
}
