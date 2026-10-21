#ifndef DECODER_H
#define DECODER_H

#include "trike_types.h"

// Decodes the error vector.
int decode(
    uint8_t *e0, uint8_t *e1, uint8_t *e2,
    const uint8_t *s, const uint32_t *idx0, const uint32_t *idx1, const uint32_t *idx2);

#endif
