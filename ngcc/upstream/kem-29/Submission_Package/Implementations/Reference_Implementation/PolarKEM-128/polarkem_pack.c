#include "polarkem_pack.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

uint16_t polarkem_compress_coefficient(uint16_t coefficient)
{
    const uint32_t scale = UINT32_C(1) << POLARKEM_QUANT_BITS;
    const uint32_t mask = scale - UINT32_C(1);
    uint32_t rounded = ((uint32_t)coefficient * scale +
                        (uint32_t)(POLARKEM_Q / 2u)) /
                       (uint32_t)POLARKEM_Q;

    return (uint16_t)(rounded & mask);
}

uint16_t polarkem_decompress_coefficient(uint16_t compressed)
{
    const uint32_t scale = UINT32_C(1) << POLARKEM_QUANT_BITS;
    uint32_t rounded = ((uint32_t)compressed * (uint32_t)POLARKEM_Q +
                        (scale / UINT32_C(2))) /
                       scale;

    return (uint16_t)rounded;
}

void polarkem_pack_coefficients(
    const uint16_t coefficients[POLARKEM_N],
    unsigned char payload[POLARKEM_CT_PAYLOAD_BYTES])
{
    uint32_t accumulator = 0u;
    unsigned int available = 0u;
    size_t out = 0u;
    size_t i;

    memset(payload, 0, POLARKEM_CT_PAYLOAD_BYTES);
    for (i = 0u; i < POLARKEM_N; ++i) {
        uint32_t value = polarkem_compress_coefficient(coefficients[i]);
        accumulator |= value << available;
        available += POLARKEM_QUANT_BITS;
        while (available >= 8u) {
            payload[out++] = (unsigned char)(accumulator & 0xffu);
            accumulator >>= 8;
            available -= 8u;
        }
    }
    if (available != 0u) {
        payload[out++] = (unsigned char)(accumulator & 0xffu);
    }
    while (out < POLARKEM_CT_PAYLOAD_BYTES) {
        payload[out++] = 0u;
    }
}

void polarkem_unpack_coefficients(
    const unsigned char payload[POLARKEM_CT_PAYLOAD_BYTES],
    uint16_t coefficients[POLARKEM_N])
{
    const uint32_t mask = (UINT32_C(1) << POLARKEM_QUANT_BITS) - UINT32_C(1);
    uint32_t accumulator = 0u;
    unsigned int available = 0u;
    size_t in = 0u;
    size_t i;

    for (i = 0u; i < POLARKEM_N; ++i) {
        while (available < POLARKEM_QUANT_BITS) {
            accumulator |= (uint32_t)payload[in++] << available;
            available += 8u;
        }
        coefficients[i] = polarkem_decompress_coefficient(
            (uint16_t)(accumulator & mask));
        accumulator >>= POLARKEM_QUANT_BITS;
        available -= POLARKEM_QUANT_BITS;
    }
}
