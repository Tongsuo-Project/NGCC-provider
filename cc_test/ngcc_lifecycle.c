/* Exercise provider key ownership without EVP retaining an extra key reference. */
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/core_dispatch.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/provider.h>

#define CHECK(condition, stage)                                                 \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "FAIL %s\n", stage);                                \
            ERR_print_errors_fp(stderr);                                       \
            goto end;                                                          \
        }                                                                      \
    } while (0)

static void *watched_key;
static atomic_uint key_frees;

static void *test_malloc(size_t size, const char *file, int line)
{
    (void)file;
    (void)line;
    return malloc(size);
}

static void *test_realloc(void *ptr, size_t size, const char *file, int line)
{
    (void)file;
    (void)line;
    return realloc(ptr, size);
}

static void test_free(void *ptr, const char *file, int line)
{
    (void)file;
    (void)line;
    if (ptr != NULL && ptr == watched_key)
        atomic_fetch_add(&key_frees, 1);
    free(ptr);
}

static const OSSL_DISPATCH *algorithm(const OSSL_ALGORITHM *algs,
                                     const char *name)
{
    size_t len = strlen(name);

    for (; algs != NULL && algs->algorithm_names != NULL; ++algs) {
        if (strncmp(algs->algorithm_names, name, len) == 0
            && (algs->algorithm_names[len] == '\0'
                || algs->algorithm_names[len] == ':'))
            return algs->implementation;
    }
    return NULL;
}

static const OSSL_DISPATCH *function(const OSSL_DISPATCH *table, int id)
{
    static const OSSL_DISPATCH missing = { 0, NULL };

    for (; table != NULL && table->function_id != 0; ++table) {
        if (table->function_id == id)
            return table;
    }
    return &missing;
}

static void *generate(void *provctx, const OSSL_DISPATCH *table)
{
    OSSL_FUNC_keymgmt_gen_init_fn *init = OSSL_FUNC_keymgmt_gen_init(
        function(table, OSSL_FUNC_KEYMGMT_GEN_INIT));
    OSSL_FUNC_keymgmt_gen_fn *gen = OSSL_FUNC_keymgmt_gen(
        function(table, OSSL_FUNC_KEYMGMT_GEN));
    OSSL_FUNC_keymgmt_gen_cleanup_fn *cleanup = OSSL_FUNC_keymgmt_gen_cleanup(
        function(table, OSSL_FUNC_KEYMGMT_GEN_CLEANUP));
    void *ctx, *key;

    if (init == NULL || gen == NULL || cleanup == NULL)
        return NULL;
    ctx = init(provctx, OSSL_KEYMGMT_SELECT_KEYPAIR, NULL);
    if (ctx == NULL)
        return NULL;
    key = gen(ctx, NULL, NULL);
    cleanup(ctx);
    return key;
}

typedef struct {
    void *ctx;
    void *(*dupctx)(void *);
    void (*freectx)(void *);
} DUP_TEST;

static void *duplicate_worker(void *arg)
{
    DUP_TEST *test = arg;
    void *copy;
    int i;

    for (i = 0; i < 1000; ++i) {
        copy = test->dupctx(test->ctx);
        if (copy == NULL)
            return test;
        test->freectx(copy);
    }
    return NULL;
}

static int concurrent_duplicates(DUP_TEST *test)
{
    pthread_t threads[8];
    void *result;
    size_t started = 0, i;
    int ok = 1;

    for (i = 0; i < sizeof(threads) / sizeof(threads[0]); ++i) {
        if (pthread_create(&threads[i], NULL, duplicate_worker, test) != 0) {
            ok = 0;
            break;
        }
        ++started;
    }
    for (i = 0; i < started; ++i) {
        if (pthread_join(threads[i], &result) != 0 || result != NULL)
            ok = 0;
    }
    return ok;
}

static int lifecycle(void *provctx, const OSSL_DISPATCH *km,
                     const OSSL_DISPATCH *op, int kem)
{
    OSSL_FUNC_keymgmt_free_fn *key_free = OSSL_FUNC_keymgmt_free(
        function(km, OSSL_FUNC_KEYMGMT_FREE));
    OSSL_FUNC_keymgmt_dup_fn *key_dup = OSSL_FUNC_keymgmt_dup(
        function(km, OSSL_FUNC_KEYMGMT_DUP));
    OSSL_FUNC_kem_encapsulate_fn *encaps = NULL;
    OSSL_FUNC_kem_decapsulate_fn *decaps = NULL;
    OSSL_FUNC_signature_digest_sign_update_fn *update = NULL;
    OSSL_FUNC_signature_digest_sign_final_fn *sign_final = NULL;
    OSSL_FUNC_signature_verify_fn *verify = NULL;
    int (*init1)(void *, void *, const OSSL_PARAM *);
    int (*init2)(void *, void *, const OSSL_PARAM *);
    DUP_TEST test = { NULL, NULL, NULL };
    void *key1 = NULL, *key2 = NULL, *key_copy = NULL;
    void *ctx1 = NULL, *ctx2 = NULL, *copy = NULL;
    unsigned char output[4096], secret[64], recovered[64];
    static const unsigned char message[] = "retained provider key";
    size_t length, secret_len, recovered_len;
    int phase, ok = 0;

    CHECK(key_free != NULL && key_dup != NULL, "key callbacks");
    if (kem) {
        OSSL_FUNC_kem_newctx_fn *newctx = OSSL_FUNC_kem_newctx(
            function(op, OSSL_FUNC_KEM_NEWCTX));

        test.dupctx = OSSL_FUNC_kem_dupctx(function(op, OSSL_FUNC_KEM_DUPCTX));
        test.freectx = OSSL_FUNC_kem_freectx(function(op, OSSL_FUNC_KEM_FREECTX));
        init1 = OSSL_FUNC_kem_encapsulate_init(function(op, OSSL_FUNC_KEM_ENCAPSULATE_INIT));
        init2 = OSSL_FUNC_kem_decapsulate_init(function(op, OSSL_FUNC_KEM_DECAPSULATE_INIT));
        encaps = OSSL_FUNC_kem_encapsulate(function(op, OSSL_FUNC_KEM_ENCAPSULATE));
        decaps = OSSL_FUNC_kem_decapsulate(function(op, OSSL_FUNC_KEM_DECAPSULATE));
        CHECK(newctx != NULL && encaps != NULL && decaps != NULL, "KEM callbacks");
        ctx1 = newctx(provctx);
        ctx2 = newctx(provctx);
    } else {
        OSSL_FUNC_signature_newctx_fn *newctx = OSSL_FUNC_signature_newctx(
            function(op, OSSL_FUNC_SIGNATURE_NEWCTX));

        test.dupctx = OSSL_FUNC_signature_dupctx(function(op, OSSL_FUNC_SIGNATURE_DUPCTX));
        test.freectx = OSSL_FUNC_signature_freectx(function(op, OSSL_FUNC_SIGNATURE_FREECTX));
        init1 = OSSL_FUNC_signature_sign_init(function(op, OSSL_FUNC_SIGNATURE_SIGN_INIT));
        init2 = OSSL_FUNC_signature_verify_init(function(op, OSSL_FUNC_SIGNATURE_VERIFY_INIT));
        update = OSSL_FUNC_signature_digest_sign_update(function(op, OSSL_FUNC_SIGNATURE_DIGEST_SIGN_UPDATE));
        sign_final = OSSL_FUNC_signature_digest_sign_final(function(op, OSSL_FUNC_SIGNATURE_DIGEST_SIGN_FINAL));
        verify = OSSL_FUNC_signature_verify(function(op, OSSL_FUNC_SIGNATURE_VERIFY));
        CHECK(newctx != NULL && update != NULL && sign_final != NULL
              && verify != NULL, "sig callbacks");
        ctx1 = newctx(provctx, NULL);
        ctx2 = newctx(provctx, NULL);
    }
    CHECK(ctx1 != NULL && ctx2 != NULL && test.dupctx != NULL
          && test.freectx != NULL && init1 != NULL && init2 != NULL,
          "operation contexts");
    copy = test.dupctx(ctx1);
    CHECK(copy != NULL, "duplicate uninitialized context");
    test.freectx(copy);
    copy = NULL;
    key1 = generate(provctx, km);
    key2 = generate(provctx, km);
    CHECK(key1 != NULL && key2 != NULL, "keygen");
    watched_key = key1;
    atomic_store(&key_frees, 0);
    key_copy = key_dup(key1, OSSL_KEYMGMT_SELECT_KEYPAIR);
    CHECK(key_copy != NULL && key_copy != key1, "explicit independent key copy");
    key_free(key_copy);
    key_copy = NULL;
    CHECK(atomic_load(&key_frees) == 0, "key copy has independent lifetime");

    for (phase = 0; phase < 2; ++phase) {
        void *key = phase == 0 ? key1 : key2;

        CHECK(init1(ctx1, key, NULL) && init1(ctx1, key, NULL), "reinitialize same key");
        CHECK(atomic_load(&key_frees) == 0, "other context retains previous key");
        CHECK(init2(ctx2, key, NULL), "initialize peer context");
        if (phase == 1) {
            CHECK(atomic_load(&key_frees) == 1, "replacement releases previous key exactly once");
            watched_key = key2;
            atomic_store(&key_frees, 0);
        }
        if (!kem)
            CHECK(update(ctx1, message, sizeof(message)), "buffer message before duplication");
        copy = test.dupctx(ctx1);
        CHECK(copy != NULL, "duplicate initialized context");
        test.freectx(ctx1);
        ctx1 = copy;
        copy = test.dupctx(ctx2);
        CHECK(copy != NULL, "duplicate peer context");
        test.freectx(ctx2);
        ctx2 = copy;
        copy = NULL;
        key_free(key);
        if (phase == 0)
            key1 = NULL;
        else
            key2 = NULL;
        CHECK(atomic_load(&key_frees) == 0, "context retains original key after caller release");
        CHECK(init1(ctx1, NULL, NULL) == 0, "failed init leaves retained key intact");
        length = sizeof(output);
        if (kem) {
            secret_len = sizeof(secret);
            recovered_len = sizeof(recovered);
            CHECK(encaps(ctx1, output, &length, secret, &secret_len), "encapsulate after release");
            CHECK(decaps(ctx2, recovered, &recovered_len, output, length), "decapsulate after release");
            CHECK(secret_len == recovered_len
                  && CRYPTO_memcmp(secret, recovered, secret_len) == 0, "shared secret");
        } else {
            CHECK(sign_final(ctx1, output, &length, sizeof(output)), "sign after release");
            CHECK(verify(ctx2, output, length, message, sizeof(message)) == 1, "verify after release");
        }
    }
    test.ctx = ctx1;
    CHECK(concurrent_duplicates(&test), "concurrent retain/release");
    CHECK(atomic_load(&key_frees) == 0, "concurrent duplicates preserve key");
    test.freectx(ctx1);
    ctx1 = NULL;
    CHECK(atomic_load(&key_frees) == 0, "peer owns remaining reference");
    test.freectx(ctx2);
    ctx2 = NULL;
    CHECK(atomic_load(&key_frees) == 1, "last context releases key exactly once");
    ok = 1;
    printf("PASS %s key references: init, dup, replace, caller release, 8000 concurrent dup/free, final release\n",
           kem ? "KEM" : "SIG");
end:
    watched_key = NULL;
    if (test.freectx != NULL) {
        test.freectx(copy);
        test.freectx(ctx1);
        test.freectx(ctx2);
    }
    if (key_free != NULL) {
        key_free(key_copy);
        key_free(key1);
        key_free(key2);
    }
    return ok;
}

int main(int argc, char **argv)
{
    OSSL_LIB_CTX *libctx = NULL;
    OSSL_PROVIDER *def = NULL, *provider = NULL;
    const OSSL_ALGORITHM *keys = NULL, *kems = NULL, *sigs = NULL;
    void *provctx;
    int no_cache, ret = 1;
    const char *kind = argc == 3 ? argv[2] : "both";

    if ((argc != 2 && argc != 3)
        || (strcmp(kind, "kem") != 0 && strcmp(kind, "sig") != 0
            && strcmp(kind, "both") != 0)
        || !CRYPTO_set_mem_functions(test_malloc, test_realloc, test_free))
        return 2;
    CHECK((libctx = OSSL_LIB_CTX_new()) != NULL, "libctx");
    CHECK(OSSL_PROVIDER_set_default_search_path(libctx, argv[1]), "module directory");
    CHECK((def = OSSL_PROVIDER_load(libctx, "default")) != NULL, "default provider");
    CHECK((provider = OSSL_PROVIDER_load(libctx, "ngcc")) != NULL, "ngcc provider");
    provctx = OSSL_PROVIDER_get0_provider_ctx(provider);
    keys = OSSL_PROVIDER_query_operation(provider, OSSL_OP_KEYMGMT, &no_cache);
    kems = OSSL_PROVIDER_query_operation(provider, OSSL_OP_KEM, &no_cache);
    sigs = OSSL_PROVIDER_query_operation(provider, OSSL_OP_SIGNATURE, &no_cache);
    if (strcmp(kind, "sig") != 0)
        CHECK(lifecycle(provctx, algorithm(keys, "kem-bw-kem-128"),
                        algorithm(kems, "kem-bw-kem-128"), 1), "KEM lifecycle");
    if (strcmp(kind, "kem") != 0)
        CHECK(lifecycle(provctx, algorithm(keys, "sig-01-aigis-sig1"),
                        algorithm(sigs, "sig-01-aigis-sig1"), 0), "sig lifecycle");
    ret = 0;
end:
    if (provider != NULL) {
        OSSL_PROVIDER_unquery_operation(provider, OSSL_OP_KEYMGMT, keys);
        OSSL_PROVIDER_unquery_operation(provider, OSSL_OP_KEM, kems);
        OSSL_PROVIDER_unquery_operation(provider, OSSL_OP_SIGNATURE, sigs);
    }
    OSSL_PROVIDER_unload(provider);
    OSSL_PROVIDER_unload(def);
    OSSL_LIB_CTX_free(libctx);
    return ret;
}
