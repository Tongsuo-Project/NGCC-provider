#ifndef TRIKE_PARAMS_H
#define TRIKE_PARAMS_H

#define DIVIDE_AND_CEIL(a, b) (((a) + (b) - 1) / (b))

#define PARAM_N0 3


#define PARAM_R 9437
#define PARAM_D 27
#define LOG2D 4
#define PARAM_T 201
#define PARAM_M 256
#define PADDED_R_SIZE 16384
#define PARAM_DELTA 2
#define COA 3234304810124655ll
#define COB 6772697599995158ll
#define PARAM_S 28
#define MAX_OVERLAP 5


#define PARAM_N (PARAM_N0 * PARAM_R)

#define R_SIZE_BYTES DIVIDE_AND_CEIL(PARAM_R, 8)
#define M_SIZE_BYTES PARAM_M / 8
#define SALT_SIZE_BYTES 16
#define N_SIZE_BYTES (R_SIZE_BYTES * PARAM_N0)

#define R_SIZE_64 DIVIDE_AND_CEIL(PARAM_R, 64)
#define R_64_LAST_BITS (((PARAM_R - 1) & 63) + 1)
#define R_64_REST_BITS (64 - R_64_LAST_BITS)
#define R_64_LAST_MASK (0xFFFFFFFFFFFFFFFFULL >> R_64_REST_BITS)

#define PADDED_R_SIZE_BYTES PADDED_R_SIZE / 8
#define PADDED_R_SIZE_64 PADDED_R_SIZE / 64

#define R_SIZE_ZMMS DIVIDE_AND_CEIL(PARAM_R, 512)
#define R_SIZE_ZMM_64 (R_SIZE_ZMMS * 8)
#define R_ZMM_SIZE_BYTES (R_SIZE_ZMMS * 64)
#define N_ZMM_SIZE_BYTES (R_ZMM_SIZE_BYTES * PARAM_N0)

#define DUP_R_ZMM_SIZE_BYTES (R_ZMM_SIZE_BYTES << 1)
#define UPC_SIZE_BYTES (R_ZMM_SIZE_BYTES * (LOG2D + 1))

#define DUP_R_SIZE_ZMM_64 (R_SIZE_ZMM_64 << 1)

#define R_8_LAST_BITS (((PARAM_R - 1) & 7) + 1)
#define R_8_LAST_MASK ((1 << R_8_LAST_BITS) - 1)

#define BIT_FLIP_ITER 7

#define HALF_R ((PARAM_R + 1) >> 1)

#endif
