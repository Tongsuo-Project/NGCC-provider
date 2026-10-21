#ifndef PARAMS_H
#define PARAMS_H
#define LORE_BACKEND_SM3 1

// Parameter-set targets, not claims about whole-KEM security.
#ifndef LORE_LEVEL
    #error "LORE_LEVEL is not defined. Please specify a level during compilation, e.g., -DLORE_LEVEL=1"
#endif

// Namespace-prefix for all public functions
#define LORE_NAMESPACE(s) lore_ref_##s


// Common parameters
#define LORE_Q 257
// Retain 32-byte symmetric values for L1/L2; L3/L4 consume the full
// 48/64-byte root seed. Long output alone does not provide high-bit security.
#if LORE_LEVEL <= 2
#define LORE_SYMBYTES 32
#elif LORE_LEVEL == 3
#define LORE_SYMBYTES 48
#else
#define LORE_SYMBYTES 64
#endif

#if LORE_LEVEL == 1
    #define LORE_N 512
    #define LORE_KAPPA 128
    #define LORE_K_VEC 4
    #define LORE_K 1
    #define LORE_T 2
    #define LORE_L 1      // Target bit length l=1 in Algorithm 4
    #define LORE_R_BITS 0 // No extra bits needed for compression when t=2
    #define LORE_HWT_P1 50
    #define LORE_HWT_M1 50
    #define LORE_HWT_P2 1
    #define LORE_HWT_M2 1
#elif LORE_LEVEL == 2
    #define LORE_N 512
    #define LORE_KAPPA 256
    #define LORE_BCH_M 9     
    #define LORE_BCH_T 28    
    #define LORE_K 2
    #define LORE_T 2
    #define LORE_L 1
    #define LORE_R_BITS 0
    #define LORE_HWT_P1 70
    #define LORE_HWT_M1 70
    #define LORE_HWT_P2 6
    #define LORE_HWT_M2 6
#elif LORE_LEVEL == 3
    #define LORE_N 512
    #define LORE_KAPPA 384
    #define LORE_BCH_M 9     
    #define LORE_BCH_T 14    
    #define LORE_K 3
    #define LORE_T 4
    #define LORE_L 1
    #define LORE_R_BITS 1 
    #define LORE_HWT_P1 90
    #define LORE_HWT_M1 90
    #define LORE_HWT_P2 20
    #define LORE_HWT_M2 20
#elif LORE_LEVEL == 4
    #define LORE_N 1024
    #define LORE_KAPPA 512
    // Match the GitHub DFR script's n=1024, kappa=512 BCH choice.
    // 64 message bytes + ceil(10 * 51 / 8) = 64 parity bytes fit 1024 bits.
    #define LORE_BCH_M 10
    #define LORE_BCH_T 51
    #define LORE_K 2
    #define LORE_T 4
    #define LORE_L 1
    #define LORE_R_BITS 1
    // The sampler draws over both polynomials jointly: global counts are
    // {-2:200, -1:400, 0:848, 1:400, 2:200}.
    #define LORE_HWT_P1 200
    #define LORE_HWT_M1 200
    #define LORE_HWT_P2 100
    #define LORE_HWT_M2 100
#else
    #error "LORE_LEVEL must be defined as 1, 2, 3, or 4"
#endif

#define LORE_MSG_BYTES (LORE_KAPPA / 8)
#if LORE_LEVEL > 1
    #define LORE_ECC_BYTES ((LORE_BCH_M * LORE_BCH_T + 7) / 8)
    #define LORE_CODEWORD_BYTES (LORE_MSG_BYTES + LORE_ECC_BYTES) 
#endif
// Total Hamming weight
#define LORE_HWT_TOTAL (LORE_HWT_P1 + LORE_HWT_M1 + LORE_HWT_P2 + LORE_HWT_M2)

#if LORE_R_BITS == 0
#define LORE_R_SIZE 1
#elif LORE_R_BITS == 1
#define LORE_R_SIZE 2
#elif LORE_R_BITS == 2
#define LORE_R_SIZE 4
#endif

// Size of the secret key in bytes 
#define LORE_SECRETKEYBYTES ( (LORE_K * 2) + (LORE_K * LORE_HWT_TOTAL * 4) + (LORE_K * LORE_POLY_BYTES) )
// Size of the compressed t-part of a polynomial in bytes
#define LORE_POLY_COMPRESSED_BYTES_T ((LORE_N * LORE_R_BITS + 7) / 8)

// Bytes occupied by the T-part of Cv (t=2 takes N/8 bytes, t=4 takes N/4 bytes)
#define LORE_CV_T_BYTES (LORE_N * (LORE_T == 2 ? 1 : 2) / 8)

// Size of the public key in bytes
#define LORE_PUBLICKEYBYTES (LORE_SYMBYTES + (LORE_K * LORE_N) + ((LORE_K * LORE_N + 7) / 8) + (LORE_K * LORE_POLY_COMPRESSED_BYTES_T) + 2)
// Manually define the size of a compressed polyvec based on LORE_K and the single poly size
#define LORE_POLYVEC_COMPRESSED_BYTES_T (LORE_K * LORE_POLY_COMPRESSED_BYTES_T)

#define LORE_T_BITS (LORE_T == 4 ? 2 : 1)
// Total size of the ciphertext in bytes
#define LORE_INDCPA_BYTES ( (LORE_K * LORE_N) + ((LORE_K * LORE_N + 7) / 8) + LORE_POLYVEC_COMPRESSED_BYTES_T + (LORE_N / 8) + LORE_CV_T_BYTES + 2)
#define LORE_CIPHERTEXTBYTES   LORE_INDCPA_BYTES
#define LORE_POLY_BYTES (LORE_N * 2)

#define LORE_KEM_SECRETKEYBYTES  (LORE_SECRETKEYBYTES + LORE_PUBLICKEYBYTES + 2*LORE_SYMBYTES)
#define LORE_KEM_PUBLICKEYBYTES  LORE_PUBLICKEYBYTES
#define LORE_BYTES               LORE_SYMBYTES

#if LORE_LEVEL == 4
#if LORE_CODEWORD_BYTES * 8 > LORE_N
#error "BCH codeword does not fit the ring polynomial"
#endif
#if LORE_HWT_TOTAL > LORE_N || LORE_K * LORE_N > 65535
#error "Fixed-weight sampler bounds are invalid"
#endif
#endif


#endif
