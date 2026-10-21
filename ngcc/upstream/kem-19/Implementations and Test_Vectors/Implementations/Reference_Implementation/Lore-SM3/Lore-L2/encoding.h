#ifndef LORE_ENCODING_H
#define LORE_ENCODING_H
#include <stdint.h>

static inline uint16_t lore_load16_le(const unsigned char *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static inline void lore_store16_le(unsigned char *p, uint16_t value)
{
    p[0] = (unsigned char)value;
    p[1] = (unsigned char)(value >> 8);
}
#endif
