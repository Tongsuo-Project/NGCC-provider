/*
 * SIG_AlgorithmInstance.h - GALAS signature interface (NGCC template).
 *
 * Implements the NGCC SIG programming interface by wiring together the GALAS
 * reference library (OWF, BAVC, VOLE, QuickSilver, Fiat-Shamir). The instance
 * is selected by the GALAS_INSTANCE macro; packaged instance directories set
 * the matching default.
 */
#ifndef SIG_ALGORITHM_INSTANCE_H
#define SIG_ALGORITHM_INSTANCE_H

/* Set 0 to generate test vectors; 1 for blank template. */
#define OUTPUT_BLANK_TEST_VECTORS 0

/* Compile with -DGALAS_INSTANCE=... or use the packaged default below. */
#ifndef GALAS_INSTANCE
#define GALAS_INSTANCE GALAS_256S
#endif
#ifndef GALAS_INSTANCE_NAME
#define GALAS_INSTANCE_NAME "Galas-256S"
#endif
#define ALGORITHM_INSTANCE GALAS_INSTANCE_NAME

#ifdef __cplusplus
extern "C" {
#endif

unsigned long long sig_get_pk_len_bytes(void);
unsigned long long sig_get_sk_len_bytes(void);
unsigned long long sig_get_sn_len_bytes(void);

int sig_keygen(unsigned char* pk, unsigned long long* pk_len_bytes,
               unsigned char* sk, unsigned long long* sk_len_bytes);

int sig_sign(unsigned char* sk, unsigned long long sk_len_bytes,
             unsigned char* m, unsigned long long m_len_bytes,
             unsigned char* sn, unsigned long long* sn_len_bytes);

int sig_verify(unsigned char* pk, unsigned long long pk_len_bytes,
               unsigned char* sn, unsigned long long sn_len_bytes,
               unsigned char* m, unsigned long long m_len_bytes);

/* Set the seed used by sig_keygen's internal DRNG (for reproducible KATs).
   Pass NULL/0 to revert to the default zero seed. */
void galas_set_kat_seed(const unsigned char* seed, unsigned long long len);

#ifdef __cplusplus
}
#endif
#endif
