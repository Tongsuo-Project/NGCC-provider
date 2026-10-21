#ifndef PARAMETERS_H
#define PARAMETERS_H

#include <stdint.h>
#include "drng.h"
#include "auxfunc.h"

#ifdef __cplusplus
extern "C"
{
#endif

  #ifndef RRLWR_SECURITY_LEVEL
  #define RRLWR_SECURITY_LEVEL 128
  #endif

  // Algorithm parameters (should not be changed)
  #if (RRLWR_SECURITY_LEVEL == 128)
    #define RRLWR_N                   (128)
    #define RRLWR_K                   (5)
    #define RRLWR_PKE_ELL             (1)
    #define RRLWR_PKE_LOGQ            (13)
    #define RRLWR_PKE_LOGP            (11)
    #define RRLWR_PKE_LOGT            (3)
    #define RRLWR_PKE_LOG_ETA         (1)
  #elif (RRLWR_SECURITY_LEVEL == 256)
    #define RRLWR_N                   (128)
    #define RRLWR_K                   (9)
    #define RRLWR_PKE_ELL             (2)
    #define RRLWR_PKE_LOGQ            (13)
    #define RRLWR_PKE_LOGP            (11)
    #define RRLWR_PKE_LOGT            (3)
    #define RRLWR_PKE_LOG_ETA         (1)
  #elif (RRLWR_SECURITY_LEVEL == 512)
    #define RRLWR_N                   (128)
    #define RRLWR_K                   (17)
    #define RRLWR_PKE_ELL             (4)
    #define RRLWR_PKE_LOGQ            (13)
    #define RRLWR_PKE_LOGP            (11)
    #define RRLWR_PKE_LOGT            (8)
    #define RRLWR_PKE_LOG_ETA         (1)
  #endif

  #define RRLWR_PKE_SEED_A_LEN        (64)
  #define RRLWR_SEED_S_LEN            (64)
  #define RRLWR_PKE_P                 ((int32_t)1 << RRLWR_PKE_LOGP)
  #define RRLWR_PKE_PACKED_POLYQ_LEN  (RRLWR_PKE_LOGQ*RRLWR_N/8)
  #define RRLWR_PKE_PACKED_POLYP_LEN  (RRLWR_PKE_LOGP*RRLWR_N/8)
  #define RRLWR_PKE_PACKED_POLYT_LEN  (RRLWR_PKE_LOGT*RRLWR_N/8)
  #define RRLWR_PKE_PACKED_POLY1_LEN  (RRLWR_N/8)
  #define RRLWR_PACKED_POLY2_LEN      (2*RRLWR_N/8)
  #define RRLWR_PACKED_POLY11_LEN     (11*RRLWR_N/8)

  #define RRLWR_PKE_MESSAGE_LEN       (RRLWR_PKE_ELL*RRLWR_N/8)
  #define RRLWR_PKE_SK_LEN            (RRLWR_K*RRLWR_PACKED_POLY2_LEN)
  #define RRLWR_PKE_PK_LEN            (RRLWR_PKE_SEED_A_LEN + RRLWR_K*RRLWR_PKE_PACKED_POLYP_LEN)
  #define RRLWR_PKE_CT_LEN            (RRLWR_PKE_ELL*RRLWR_PKE_PACKED_POLYT_LEN + RRLWR_K*RRLWR_PKE_PACKED_POLYP_LEN)

  #define RRLWR_KEM_SEED_Z_LEN        (RRLWR_PKE_MESSAGE_LEN)
  #define RRLWR_KEM_HPK_LEN           (RRLWR_PKE_MESSAGE_LEN)
  #define RRLWR_KEM_SS_LEN            (RRLWR_PKE_MESSAGE_LEN)
  #define RRLWR_KEM_SK_LEN            (RRLWR_PKE_SK_LEN + RRLWR_PKE_PK_LEN + RRLWR_KEM_HPK_LEN + RRLWR_KEM_SEED_Z_LEN)
  #define RRLWR_KEM_PK_LEN            (RRLWR_PKE_PK_LEN)
  #define RRLWR_KEM_CT_LEN            (RRLWR_PKE_CT_LEN)

  // Define hash functions
  #define RRLWR_SM3_XOF(output, output_len, input, input_len) \
          pseudoXOF(8ULL*(unsigned long long)(output_len), \
                    (const unsigned char *)(input), \
                    8ULL*(unsigned long long)(input_len), \
                    (unsigned char *)(output))
  #define RRLWR_XOF(output, output_len, input, input_len) \
          RRLWR_SM3_XOF((output), (output_len), (input), (input_len))
  #define GENERATE_RANDOM_BYTES(output, output_len, ctx) \
          get_random_number(ctx, output, 8*(output_len))

  #define RRLWR_PKE_Q                 ((int32_t)1 << RRLWR_PKE_LOGQ)

#ifdef __cplusplus
}
#endif

#endif
