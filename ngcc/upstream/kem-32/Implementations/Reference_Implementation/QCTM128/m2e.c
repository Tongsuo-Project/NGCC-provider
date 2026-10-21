#include <stdlib.h>
#include <string.h>

#include "m2e.h"
#include "rng.h"

static int fixed_weight_from_bits(const unsigned char *bits, int *error)
{
    int count = 0;
    int j;
    unsigned char *seen;

    if (bits == NULL || error == NULL) {
        return FAIL;
    }

    for (j = 0; j < PARAM_RHO && count < NB_ERRORS; j++) {
        int i;
        int d = 0;

        for (i = 0; i < EXT_DEGREE; i++) {
            if (ind(bits, (size_t)PARAM_SIGMA1 * (size_t)j + (size_t)i)) {
                d |= 1 << i;
            }
        }
        if (d < LENGTH) {
            error[count++] = d;
        }
    }

    if (count < NB_ERRORS) {
        return FAIL;
    }

    seen = calloc((size_t)LENGTH, 1);
    if (seen == NULL) {
        return FAIL;
    }
    for (j = 0; j < NB_ERRORS; j++) {
        if (seen[error[j]] != 0) {
            free(seen);
            return FAIL;
        }
        seen[error[j]] = 1;
    }
    free(seen);
    return SUCCESS;
}

int fixed_weight_random(OUT int *error)
{
    unsigned char *bits;
    int status;

    bits = malloc(FIXED_WEIGHT_BYTES);
    if (bits == NULL) {
        return FAIL;
    }

    do {
        randombytes(bits, FIXED_WEIGHT_BYTES);
        status = fixed_weight_from_bits(bits, error);
    } while (status != SUCCESS);

    free(bits);
    return SUCCESS;
}

int m2error(IN unsigned char *m, OUT int *error)
{
    return fixed_weight_from_bits(m, error);
}
