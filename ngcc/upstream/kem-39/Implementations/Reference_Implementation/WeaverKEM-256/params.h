#ifndef PARAMS_H
#define PARAMS_H

#ifndef WEAVER_MODE
#define WEAVER_MODE 5  /* 1: 512, 3: 1024, 5: 2048 */
#endif

#define PK_COMPRESS

// For Debug:
//#define NO_INV_Q_LIFTING

#if   (WEAVER_MODE == 1)
  #define WEAVER_NAMESPACE(s) weaver640_ref##s
  /* Table 2 core: (n,k,q,eta1,eta2,dt,du,dv) = (128,5,3329,3,2,9,9,6). */
  #define WEAVER_INDCPA_MSGBYTES 16
  #define WEAVER_N 128
  #define WEAVER_K 5
  #define WEAVER_Q 3329
  #define WEAVER_QBITS 12
  #define WEAVER_ETA1 3
  #define WEAVER_ETA2 2
  #define WEAVER_DT 9
  #define WEAVER_DU 9
  #define WEAVER_DV 6
  /* SYMBYTES: length of publicseed or keygen seed */
  #define WEAVER_SYMBYTES 32
  /* HBYTES: length of public key hash */
  #define WEAVER_HBYTES 32

#elif (WEAVER_MODE == 3)
  #define WEAVER_NAMESPACE(s) weaver1024_ref##s
  /* Table 2 core: (n,k,q,eta1,eta2,dt,du,dv) = (256,4,7681,7,7,10,10,8). */
  #define WEAVER_INDCPA_MSGBYTES  32
  #define WEAVER_N 256
  #define WEAVER_K 4
  #define WEAVER_Q 7681
  #define WEAVER_QBITS 13
  #define WEAVER_ETA1 7
  #define WEAVER_ETA2 7
  #define WEAVER_DT 10
  #define WEAVER_DU 10
  #define WEAVER_DV 8
  /* SYMBYTES: length of publicseed or keygen seed */
  #define WEAVER_SYMBYTES 32
  /* HBYTES: length of public key hash */
  #define WEAVER_HBYTES 64

#elif (WEAVER_MODE == 5)
  #define WEAVER_NAMESPACE(s) weaver2048_ref##s
  /* Table 2 core: (n,k,q,eta1,eta2,dt,du,dv) = (512,4,7681,9,9,11,11,9). */
  #define WEAVER_INDCPA_MSGBYTES  64
  #define WEAVER_N 512
  #define WEAVER_K 4
  #define WEAVER_Q 7681
  #define WEAVER_QBITS 13
  #define WEAVER_ETA1 9
  #define WEAVER_ETA2 9
  #define WEAVER_DT 11
  #define WEAVER_DU 11
  #define WEAVER_DV 9
  /* SYMBYTES: length of publicseed or keygen seed */
  #define WEAVER_SYMBYTES 64
  /* HBYTES: length of public key hash */
  #define WEAVER_HBYTES 128

#else
  #error "WEAVER_MODE must be in {1,3,5}"
#endif

#define WEAVER_PK_POLYVECBYTES         (WEAVER_K * ((WEAVER_N * WEAVER_DT) / 8))
#define WEAVER_POLYVECCOMPRESSEDBYTES  (WEAVER_K * ((WEAVER_N * WEAVER_DU) / 8))
#define WEAVER_POLYCOMPRESSEDBYTES    ((WEAVER_N * WEAVER_DV) / 8)
#define WEAVER_POLYBYTES     ((WEAVER_N * WEAVER_QBITS) / 8)
#define WEAVER_POLYVECBYTES  (WEAVER_K * WEAVER_POLYBYTES)

#ifndef PK_COMPRESS
#undef WEAVER_PK_POLYVECBYTES
#define WEAVER_PK_POLYVECBYTES WEAVER_POLYVECBYTES
#endif

/* ====================================================================
 * Global Parameters: DO NOT modify.
 * ==================================================================== */
#define WEAVER_HALFQ ((WEAVER_Q - 1) / 2)

/* size in bytes of shared key */
#define WEAVER_SSBYTES  WEAVER_INDCPA_MSGBYTES

/* key pair and ciphertext sizes */
#define WEAVER_INDCPA_PUBLICKEYBYTES (WEAVER_PK_POLYVECBYTES + WEAVER_SYMBYTES)
#define WEAVER_INDCPA_SECRETKEYBYTES (WEAVER_POLYVECBYTES)
#define WEAVER_INDCPA_BYTES          (WEAVER_POLYVECCOMPRESSEDBYTES + WEAVER_POLYCOMPRESSEDBYTES)

/* size in bytes of hashes, seeds, and FO outputs */
#define WEAVER_KEM_DERAND_COINBYTES WEAVER_INDCPA_MSGBYTES
#define WEAVER_GBYTES (WEAVER_SSBYTES + WEAVER_SYMBYTES)

/* helper sizes for KEM secret-key layout */
#define WEAVER_SK_HPK_OFFSET   (WEAVER_INDCPA_SECRETKEYBYTES + WEAVER_INDCPA_PUBLICKEYBYTES)
#define WEAVER_SK_Z_OFFSET     (WEAVER_SK_HPK_OFFSET + WEAVER_HBYTES)

#define WEAVER_PUBLICKEYBYTES  (WEAVER_INDCPA_PUBLICKEYBYTES)
#define WEAVER_SECRETKEYBYTES  (WEAVER_SK_Z_OFFSET + WEAVER_SYMBYTES)
#define WEAVER_CIPHERTEXTBYTES  WEAVER_INDCPA_BYTES


#endif
