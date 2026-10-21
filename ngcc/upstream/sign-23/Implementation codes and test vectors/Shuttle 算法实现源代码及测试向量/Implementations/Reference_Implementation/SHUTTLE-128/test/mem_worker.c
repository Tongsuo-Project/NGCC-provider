/*
 * mem_worker.c -- a minimal one-shot keygen + sign + verify worker for the
 * peak-memory measurement.  Run under massif:
 *   valgrind --tool=massif --stacks=yes --massif-out-file=massif.out \
 *            --quiet ./mem_worker
 * The massif.out parser (tools/peak_mem.sh) then takes the max mem_heap_B
 * and mem_stacks_B over the run.
 *
 * Fallback (massif absent): the worker self-reports its peak RSS via
 * getrusage(RUSAGE_SELF).ru_maxrss (KiB) on the LAST line of stdout as
 *   maxrss_kib=<n>
 * so tools/peak_mem.sh can fall back to it and label peak_source=maxrss.
 *
 * Exactly ONE keygen+sign+verify cycle is run (NWORK, override-able) so
 * the massif heap/stack high-water reflects a single signature path, not a
 * benchmark loop (a benchmark loop would still plateau, but one cycle is
 * the cleanest peak).  A non-zero return from any sig_* aborts.
 *
 * Built only as a standalone main under -DMEM_WORKER_MAIN.
 */
#if !defined(_POSIX_C_SOURCE) || (_POSIX_C_SOURCE < 200112L)
#    undef _POSIX_C_SOURCE
#    define _POSIX_C_SOURCE 200112L
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>

#include "SIG_AlgorithmInstance.h"
#include "drng.h"
#include "params.h"

#ifndef NWORK
#    define NWORK 1
#endif
#define MLEN 64

/* The scheme-internal DRBG (the NGCC adapter draws xi/rnd from it). */
DRNG_ctx drng_algorithm;

#ifdef MEM_WORKER_MAIN
int main(void)
{
    const unsigned long long pk_len = sig_get_pk_len_bytes();
    const unsigned long long sk_len = sig_get_sk_len_bytes();
    const unsigned long long sn_cap = sig_get_sn_len_bytes();
    uint8_t *pk = malloc((size_t)pk_len);
    uint8_t *sk = malloc((size_t)sk_len);
    uint8_t *sn = malloc((size_t)sn_cap);
    uint8_t msg[MLEN];
    uint8_t nonce[64];
    struct rusage ru;
    int i;

    if (!pk || !sk || !sn) {
        fprintf(stderr, "mem_worker: malloc failed\n");
        return 2;
    }
    for (i = 0; i < 16; ++i)
        memcpy(nonce + 4 * i, "memw", 4);
    init_random_number(&drng_algorithm, nonce, 64);
    memset(msg, 0xA5, sizeof msg);

    for (i = 0; i < NWORK; ++i) {
        unsigned long long pkl = pk_len, skl = sk_len, snl = sn_cap;
        int v;
        if (sig_keygen(pk, &pkl, sk, &skl) != 0) {
            fprintf(stderr, "mem_worker: keygen failed\n");
            return 2;
        }
        if (sig_sign(sk, sk_len, msg, MLEN, sn, &snl) != 0) {
            fprintf(stderr, "mem_worker: sign failed\n");
            return 2;
        }
        v = sig_verify(pk, pk_len, sn, snl, msg, MLEN);
        if (v != 0) {
            fprintf(stderr, "mem_worker: verify returned %d\n", v);
            return 2;
        }
    }

    printf("mode=%d pk=%llu sk=%llu sn_cap=%llu cycles=%d\n", (int)LAMBDA,
           pk_len, sk_len, sn_cap, NWORK);
    if (getrusage(RUSAGE_SELF, &ru) == 0)
        printf("maxrss_kib=%ld\n", ru.ru_maxrss);
    else
        printf("maxrss_kib=0\n");

    free(pk);
    free(sk);
    free(sn);
    return 0;
}
#endif /* MEM_WORKER_MAIN */
