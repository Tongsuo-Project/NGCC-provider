/* Compile once per KEM instance; NGCC_REF_* come from its build parameters. */
#include "ngcc_ref.h"
#include "link_kem.h"

extern int kem_keygen(unsigned char *pk, unsigned long long *pk_len,
                      unsigned char *sk, unsigned long long *sk_len);
extern int kem_enc(unsigned char *pk, unsigned long long pk_len,
                   unsigned char *ss, unsigned long long *ss_len,
                   unsigned char *ct, unsigned long long *ct_len);
extern int kem_dec(unsigned char *sk, unsigned long long sk_len,
                   unsigned char *ct, unsigned long long ct_len,
                   unsigned char *ss, unsigned long long *ss_len);

/* getter is the instance symbol; algorithm/id/instance identify its ref
 * implementation, and bits is the proposal's declared security strength. */
#define NGCC_KEM_REF_IMPL(getter, algorithm, proposal, instance, bits)         \
NGCC_SCHEME *getter(void)                                                      \
{                                                                              \
    static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;                   \
    static NGCC_SCHEME scheme;                                                 \
    const ngcc_meta_kem_t *meta;                                               \
                                                                               \
    pthread_mutex_lock(&lock);                                                 \
    if (scheme.name == NULL) {                                                 \
        meta = (const void *)ngcc_meta();                                      \
        NGCC_SCHEME initial = {                                                \
            algorithm, proposal, instance, 1, bits,                            \
            meta->pk_len, meta->sk_len, meta->ct_len, meta->ss_len,            \
            kem_keygen, kem_enc, kem_dec, NULL, NULL, ngcc_seed, &lock         \
        };                                                                     \
        scheme = initial;                                                      \
    }                                                                          \
    pthread_mutex_unlock(&lock);                                               \
    return &scheme;                                                            \
}

NGCC_KEM_REF_IMPL(NGCC_REF_SYMBOL, NGCC_REF_ALGORITHM, NGCC_REF_PROPOSAL,
                 NGCC_REF_INSTANCE, NGCC_REF_SECURITY_BITS)
