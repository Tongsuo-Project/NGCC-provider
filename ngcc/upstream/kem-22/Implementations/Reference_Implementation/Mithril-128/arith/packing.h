#ifndef PACKING_H
#define PACKING_H

#include "parameters.h"
#include "ring.h"

#ifdef __cplusplus
extern "C"
{
#endif

  void poly_pack(unsigned char *b, poly *r, int32_t bitlen);
  void poly_pack_ciphertext_t_from_acc_msg(unsigned char *ct,
                                           const uint16_t acc[RRLWR_N],
                                           const unsigned char msg[RRLWR_PKE_MESSAGE_LEN],
                                           unsigned int out);
  void ring_pack(unsigned char *b, ring_element *r, int32_t bitlen);
  void poly_unpack(poly *r, const unsigned char *b, int32_t bitlen);
  void ring_unpack(ring_element *r, const unsigned char *b, int32_t bitlen);

#ifdef __cplusplus
}
#endif

#endif
