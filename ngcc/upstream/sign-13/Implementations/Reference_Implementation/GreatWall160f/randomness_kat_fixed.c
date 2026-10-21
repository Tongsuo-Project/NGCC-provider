/* Test-only: KAT_SIG seeds this SM3 DRNG once for each recorded case. */
#include "randomness.h"
#include "drng.h"
extern DRNG_ctx drng_algorithm;
int rand_bytes(uint8_t* output, size_t bytes)
{
    return get_random_number(&drng_algorithm, output, bytes * 8);
}
