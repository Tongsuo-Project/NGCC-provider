#ifndef POLARKEM_PACK_H
#define POLARKEM_PACK_H

#include <stdint.h>

#include "polarkem_params.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Quantize one canonical coefficient modulo q.
 *
 * @param[in] coefficient Integer in [0, POLARKEM_Q - 1].
 * @return POLARKEM_QUANT_BITS-bit round-half-up quantization modulo 2^d.
 */
uint16_t polarkem_compress_coefficient(uint16_t coefficient);

/**
 * Dequantize one d-bit coefficient using round-half-up inverse scaling.
 *
 * @param[in] compressed Integer in [0, 2^POLARKEM_QUANT_BITS - 1].
 * @return Canonical representative in [0, POLARKEM_Q - 1].
 */
uint16_t polarkem_decompress_coefficient(uint16_t compressed);

/**
 * Quantize and pack POLARKEM_N coefficients into the canonical LSB-first
 * bitstream.
 *
 * @param[in]  coefficients POLARKEM_N canonical coefficients modulo q.
 * @param[out] payload      POLARKEM_CT_PAYLOAD_BYTES initialized bytes.
 * @return Nothing; the function writes exactly the payload bytes above.
 */
void polarkem_pack_coefficients(
    const uint16_t coefficients[POLARKEM_N],
    unsigned char payload[POLARKEM_CT_PAYLOAD_BYTES]);

/**
 * Unpack and dequantize the canonical LSB-first coefficient bitstream.
 *
 * @param[in]  payload      POLARKEM_CT_PAYLOAD_BYTES input bytes.
 * @param[out] coefficients POLARKEM_N canonical coefficients modulo q.
 * @return Nothing; the function writes exactly POLARKEM_N coefficients.
 */
void polarkem_unpack_coefficients(
    const unsigned char payload[POLARKEM_CT_PAYLOAD_BYTES],
    uint16_t coefficients[POLARKEM_N]);

#ifdef __cplusplus
}
#endif

#endif
