#include "lore_hash.h"

void lore_frame_header(uint8_t out[LORE_FRAME_BYTES], uint8_t domain,
                       uint32_t counter, size_t inlen)
{
    const uint64_t length = (uint64_t)inlen;
    out[0] = 'L'; out[1] = 'O'; out[2] = 'R'; out[3] = 'E';
    out[4] = 1; /* Call-layer format version. */
#ifdef LORE_BACKEND_SM3
    out[5] = 2;
#else
    out[5] = 1;
#endif
    out[6] = LORE_LEVEL;
    out[7] = domain;
    for (unsigned int i = 0; i < 8; ++i)
        out[8 + i] = (uint8_t)(length >> (56 - 8 * i));
    for (unsigned int i = 0; i < 4; ++i)
        out[16 + i] = (uint8_t)(counter >> (24 - 8 * i));
}
