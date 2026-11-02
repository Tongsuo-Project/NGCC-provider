/* Cross-check author implementations through the provider's existing ABI. */
#include "ngcc_ref.h"
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void *alloc(size_t n)
{
    void *p = NULL;
    if (posix_memalign(&p, 64, n + 64) != 0) abort();
    memset(p, 0, n + 64);
    return p;
}

static int check(NGCC_SCHEME *impl[2])
{
    NGCC_SCHEME *shape = impl[0];
    unsigned char seed[48], message[59];
    unsigned char *pk = alloc(shape->pk_len), *sk = alloc(shape->sk_len);
    unsigned char *out = alloc(shape->output_len);
    unsigned char *ss = alloc(shape->ss_len), *ss2 = alloc(shape->ss_len);
    int ok = 1;
    for (int key = 0; key < 2; key++) {
        unsigned long long pklen = shape->pk_len, sklen = shape->sk_len;
        for (size_t i = 0; i < sizeof(seed); i++) seed[i] = (unsigned char)(i + 17 * key);
        if (impl[key]->seed(seed, sizeof(seed)) != 0 ||
            impl[key]->keygen(pk, &pklen, sk, &sklen) != 0 ||
            pklen != shape->pk_len || sklen != shape->sk_len) {
            fprintf(stderr, "FAIL keygen=%d\n", key); ok = 0; continue;
        }
        for (int op = 0; op < 2; op++) {
            unsigned long long outlen = shape->output_len, sslen = shape->ss_len;
            for (size_t i = 0; i < sizeof(message); i++) message[i] = (unsigned char)(i ^ key ^ op);
            seed[0] = (unsigned char)(0xa5 ^ key ^ op);
            if (impl[op]->seed(seed, sizeof(seed)) != 0) { ok = 0; continue; }
            if (shape->type == 1) {
                if (impl[op]->enc(pk, pklen, ss, &sslen, out, &outlen) != 0 ||
                    outlen != shape->output_len || sslen != shape->ss_len) {
                    fprintf(stderr, "FAIL enc key=%d op=%d\n", key, op); ok = 0; continue;
                }
            } else if (impl[op]->sign(sk, sklen, message, sizeof(message), out, &outlen) != 0 ||
                       outlen > shape->output_len) {
                fprintf(stderr, "FAIL sign key=%d op=%d\n", key, op); ok = 0; continue;
            }
            for (int verify = 0; verify < 2; verify++) {
                int valid;
                if (shape->type == 1) {
                    unsigned long long ss2len = shape->ss_len;
                    valid = impl[verify]->dec(sk, sklen, out, outlen, ss2, &ss2len) == 0 &&
                            sslen == ss2len && memcmp(ss, ss2, sslen) == 0;
                } else {
                    valid = impl[verify]->verify(pk, pklen, out, outlen, message, sizeof(message)) == 0;
                }
                printf("%s key=%d operation=%d verification=%d\n", valid ? "PASS" : "FAIL", key, op, verify);
                fflush(stdout);
                if (!valid) ok = 0;
                if (shape->type == 2) {
                    message[0] ^= 1;
                    if (impl[verify]->verify(pk, pklen, out, outlen, message, sizeof(message)) == 0) {
                        fprintf(stderr, "FAIL altered-message rejection key=%d op=%d verify=%d\n", key, op, verify);
                        ok = 0;
                    }
                    message[0] ^= 1;
                }
            }
        }
    }
    free(pk); free(sk); free(out); free(ss); free(ss2);
    return ok;
}

int main(int argc, char **argv)
{
    NGCC_SCHEME *impl[2];
    void *handles[2];
    if (argc != 4) return 2;
    for (int i = 0; i < 2; i++) {
        NGCC_SCHEME *(*get)(void);
        handles[i] = dlopen(argv[i + 1], RTLD_NOW | RTLD_LOCAL);
        if (!handles[i]) { fprintf(stderr, "%s\n", dlerror()); return 2; }
        get = (NGCC_SCHEME *(*)(void))dlsym(handles[i], argv[3]);
        if (!get) { fprintf(stderr, "%s\n", dlerror()); return 2; }
        impl[i] = get();
    }
    if (impl[0]->type != impl[1]->type || impl[0]->pk_len != impl[1]->pk_len ||
        impl[0]->sk_len != impl[1]->sk_len || impl[0]->output_len != impl[1]->output_len ||
        impl[0]->ss_len != impl[1]->ss_len) {
        fprintf(stderr, "ABI sizes differ: ref=%zu/%zu/%zu/%zu opt=%zu/%zu/%zu/%zu\n",
                impl[0]->pk_len, impl[0]->sk_len, impl[0]->output_len, impl[0]->ss_len,
                impl[1]->pk_len, impl[1]->sk_len, impl[1]->output_len, impl[1]->ss_len);
        return 1;
    }
    int ok = check(impl);
    for (int i = 0; i < 2; i++) dlclose(handles[i]);
    return ok ? 0 : 1;
}
