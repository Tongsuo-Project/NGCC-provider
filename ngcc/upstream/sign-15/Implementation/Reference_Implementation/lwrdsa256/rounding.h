#ifndef ROUNDING_H
#define ROUNDING_H

#include <stdint.h>

uint32_t decompose_keygen(const uint32_t a, uint32_t *a0);
uint32_t decompose_sign(uint32_t a, uint32_t *a0);
unsigned int make_hint(const uint32_t a, const uint32_t b); 
uint32_t use_hint(const uint32_t a, const unsigned int hint);

#endif
