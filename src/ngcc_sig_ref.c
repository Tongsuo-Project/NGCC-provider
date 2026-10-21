/* Compile once per sig instance, with its original author sources. */
#include "ngcc_ref.h"
#include "link_sig.h"

extern int sig_keygen(unsigned char *pk, unsigned long long *pk_len,
                      unsigned char *sk, unsigned long long *sk_len);
extern int sig_sign(unsigned char *sk, unsigned long long sk_len,
                    unsigned char *message, unsigned long long message_len,
                    unsigned char *sig, unsigned long long *sig_len);
extern int sig_verify(unsigned char *pk, unsigned long long pk_len,
                      unsigned char *sig, unsigned long long sig_len,
                      unsigned char *message, unsigned long long message_len);

/* getter is the instance symbol; algorithm/id/instance identify its ref
 * implementation, and bits is the proposal's declared security strength. */
#define NGCC_SIG_REF_IMPL(getter, algorithm, proposal, instance, bits)         \
NGCC_SCHEME *getter(void)                                                      \
{                                                                              \
    static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;                   \
    static NGCC_SCHEME scheme;                                                 \
    const ngcc_meta_sig_t *meta;                                               \
                                                                               \
    pthread_mutex_lock(&lock);                                                 \
    if (scheme.name == NULL) {                                                 \
        meta = (const void *)ngcc_meta();                                      \
        NGCC_SCHEME initial = {                                                \
            algorithm, proposal, instance, 2, bits,                            \
            meta->pk_len, meta->sk_len, meta->sn_len, 0,                       \
            sig_keygen, NULL, NULL, sig_sign, sig_verify, ngcc_seed, &lock     \
        };                                                                     \
        scheme = initial;                                                      \
    }                                                                          \
    pthread_mutex_unlock(&lock);                                               \
    return &scheme;                                                            \
}

NGCC_SIG_REF_IMPL(NGCC_REF_SYMBOL, NGCC_REF_ALGORITHM, NGCC_REF_PROPOSAL,
                       NGCC_REF_INSTANCE, NGCC_REF_SECURITY_BITS)
