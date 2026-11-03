/* Micro-benchmark one NGCC instance: KEM (keygen/encaps/decaps) or
 * signature (keygen/sign/verify).  Runs one instance per process so an
 * author-code crash identifies its instance.
 *
 * Usage: ngcc_bench module-directory kem|sig algorithm iterations
 * Prints median nanoseconds per operation. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <openssl/evp.h>
#include <openssl/provider.h>
#include <openssl/core_names.h>
#include <openssl/err.h>
#include <openssl/params.h>

static double now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

static int cmp_double(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

static double median(double *v, int n)
{
    qsort(v, (size_t)n, sizeof(double), cmp_double);
    return (n & 1) ? v[n / 2] : 0.5 * (v[n / 2 - 1] + v[n / 2]);
}

int main(int argc, char **argv)
{
    OSSL_LIB_CTX *lib = NULL;
    OSSL_PROVIDER *def = NULL, *ngcc = NULL;
    EVP_PKEY_CTX *kg = NULL, *op1 = NULL, *op2 = NULL;
    EVP_PKEY *key = NULL, *pub = NULL;
    OSSL_PARAM *params = NULL;
    const OSSL_PARAM *pk;
    unsigned char msg[64], *buf1 = NULL, *buf2 = NULL;
    size_t len1 = 0, len2 = 0, cap = 0;
    const char *name, *kind;
    int iters, i, ret = 1, is_kem;
    double *t1 = NULL, *t2 = NULL, *t3 = NULL;

    setbuf(stdout, NULL);
    if (argc != 5) {
        fprintf(stderr, "usage: ngcc_bench module-directory kem|sig algorithm iterations\n");
        return 2;
    }
    kind = argv[2];
    name = argv[3];
    iters = atoi(argv[4]);
    if (iters < 1) iters = 1;
    is_kem = !strcmp(kind, "kem");
    for (i = 0; i < (int)sizeof(msg); i++) msg[i] = (unsigned char)(i * 11 + 5);

    t1 = malloc(sizeof(double) * (size_t)iters);
    t2 = malloc(sizeof(double) * (size_t)iters);
    t3 = malloc(sizeof(double) * (size_t)iters);
    if (!t1 || !t2 || !t3) goto end;

    if (!(lib = OSSL_LIB_CTX_new())) goto end;
    if (!OSSL_PROVIDER_set_default_search_path(lib, argv[1])) goto end;
    if (!(def = OSSL_PROVIDER_load(lib, "default"))) goto end;
    if (!(ngcc = OSSL_PROVIDER_load(lib, "ngcc"))) goto end;

    /* Warm up keygen so one-time tables are not counted in every sample. */
    if (!(kg = EVP_PKEY_CTX_new_from_name(lib, name, "provider=ngcc"))) goto end;
    if (EVP_PKEY_keygen_init(kg) <= 0) goto end;
    if (EVP_PKEY_generate(kg, &key) <= 0) goto end;

    for (i = 0; i < iters; i++) {
        EVP_PKEY *k = NULL;
        double t0 = now_ns();
        if (EVP_PKEY_generate(kg, &k) <= 0) goto end;
        t1[i] = now_ns() - t0;
        if (i == 0) key = k; else EVP_PKEY_free(k);
    }

    if (EVP_PKEY_todata(key, EVP_PKEY_PUBLIC_KEY, &params) <= 0) goto end;
    pk = OSSL_PARAM_locate_const(params, OSSL_PKEY_PARAM_PUB_KEY);
    if (!pk) goto end;
    {
        EVP_PKEY_CTX *ic = EVP_PKEY_CTX_new_from_name(lib, name, "provider=ngcc");
        if (!ic) goto end;
        if (EVP_PKEY_fromdata_init(ic) <= 0
            || EVP_PKEY_fromdata(ic, &pub, EVP_PKEY_PUBLIC_KEY, params) <= 0) {
            EVP_PKEY_CTX_free(ic);
            goto end;
        }
        EVP_PKEY_CTX_free(ic);
    }

    if (is_kem) {
        if (!(op1 = EVP_PKEY_CTX_new_from_pkey(lib, pub, "provider=ngcc"))) goto end;
        if (!(op2 = EVP_PKEY_CTX_new_from_pkey(lib, key, "provider=ngcc"))) goto end;
        if (EVP_PKEY_encapsulate_init(op1, NULL) <= 0) goto end;
        if (EVP_PKEY_decapsulate_init(op2, NULL) <= 0) goto end;
        if (EVP_PKEY_encapsulate(op1, NULL, &cap, NULL, &len2) <= 0 || !cap || !len2) goto end;
        if (!(buf1 = OPENSSL_malloc(cap)) || !(buf2 = OPENSSL_malloc(len2))) goto end;
        {
            size_t cl = cap, sl = len2;
            if (EVP_PKEY_encapsulate(op1, buf1, &cl, buf2, &sl) <= 0) goto end;
        }
        for (i = 0; i < iters; i++) {
            double t0 = now_ns();
            size_t cl = cap, sl = len2;
            if (EVP_PKEY_encapsulate(op1, buf1, &cl, buf2, &sl) <= 0) goto end;
            t2[i] = now_ns() - t0;
        }
        for (i = 0; i < iters; i++) {
            double t0 = now_ns();
            size_t sl = len2;
            if (EVP_PKEY_decapsulate(op2, buf2, &sl, buf1, cap) <= 0) goto end;
            t3[i] = now_ns() - t0;
        }
        printf("%s kem pk=%zu ct=%zu ss=%zu keygen_ns=%.0f encaps_ns=%.0f decaps_ns=%.0f\n",
               name, pk->data_size, cap, len2, median(t1, iters), median(t2, iters),
               median(t3, iters));
    } else {
        if (!(op1 = EVP_PKEY_CTX_new_from_pkey(lib, key, "provider=ngcc"))) goto end;
        if (!(op2 = EVP_PKEY_CTX_new_from_pkey(lib, pub, "provider=ngcc"))) goto end;
        if (EVP_PKEY_sign_init(op1) <= 0) goto end;
        if (EVP_PKEY_verify_init(op2) <= 0) goto end;
        if (EVP_PKEY_sign(op1, NULL, &cap, msg, sizeof(msg)) <= 0) goto end;
        if (!(buf1 = OPENSSL_malloc(cap + 1))) goto end;
        len1 = cap;   /* actual signature length; some schemes are variable-length */
        if (EVP_PKEY_sign(op1, buf1, &len1, msg, sizeof(msg)) <= 0) goto end;
        if (EVP_PKEY_verify(op2, buf1, len1, msg, sizeof(msg)) != 1) goto end;
        for (i = 0; i < iters; i++) {
            double t0 = now_ns();
            size_t sl = cap;
            if (EVP_PKEY_sign(op1, buf1, &sl, msg, sizeof(msg)) <= 0) goto end;
            t2[i] = now_ns() - t0;
        }
        for (i = 0; i < iters; i++) {
            double t0 = now_ns();
            if (EVP_PKEY_verify(op2, buf1, len1, msg, sizeof(msg)) != 1) goto end;
            t3[i] = now_ns() - t0;
        }
        printf("%s sig pk=%zu sig=%zu keygen_ns=%.0f sign_ns=%.0f verify_ns=%.0f\n",
               name, pk->data_size, cap, median(t1, iters), median(t2, iters),
               median(t3, iters));
    }
    ret = 0;

end:
    if (ret) {
        fprintf(stderr, "FAIL %s\n", name);
        ERR_print_errors_fp(stderr);
    }
    OPENSSL_free(buf1);
    OPENSSL_free(buf2);
    OSSL_PARAM_free(params);
    EVP_PKEY_CTX_free(kg);
    EVP_PKEY_CTX_free(op1);
    EVP_PKEY_CTX_free(op2);
    EVP_PKEY_free(key);
    EVP_PKEY_free(pub);
    OSSL_PROVIDER_unload(ngcc);
    OSSL_PROVIDER_unload(def);
    OSSL_LIB_CTX_free(lib);
    free(t1); free(t2); free(t3);
    return ret;
}