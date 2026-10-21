#include "randomness.h"
#if GREATWALL_USE_OS_RANDOM
#include "randomness_os.c"
#else
#include "drng.h"

extern DRNG_ctx drng_algorithm;

int rand_bytes(uint8_t* dst, size_t len) {
  return get_random_number(&drng_algorithm, dst, len * 8);
}
#endif
