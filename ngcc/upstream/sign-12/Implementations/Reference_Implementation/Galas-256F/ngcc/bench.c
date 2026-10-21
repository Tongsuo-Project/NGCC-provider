/*
 * bench.c - x86 self-evaluation benchmark for the GALAS reference implementation.
 *
 * The harness uses a deterministic 64-byte message, checks key generation,
 * signing, and verification, and reports average latency, average cycle count,
 * throughput, sizes, and peak resident set size.  The implementation under test
 * remains the portable C reference code; this benchmark harness is x86/Linux
 * measurement code.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <time.h>

#if defined(__x86_64__) || defined(__i386__)
#include <x86intrin.h>
static unsigned long long read_cycles(void)
{
    return __rdtsc();
}
#else
static unsigned long long read_cycles(void)
{
    return 0;
}
#endif

#include "SIG_AlgorithmInstance.h"

static double seconds_between(struct timespec start, struct timespec end)
{
    return (double)(end.tv_sec - start.tv_sec) +
           (double)(end.tv_nsec - start.tv_nsec) / 1000000000.0;
}

static long peak_rss_kb(void)
{
    struct rusage ru;
    if (getrusage(RUSAGE_SELF, &ru) != 0)
        return -1;
    return ru.ru_maxrss;
}

static void fill_message(unsigned char msg[64])
{
    for (size_t i = 0; i < 64; ++i)
        msg[i] = (unsigned char)(0x40u + ((17u * (unsigned)i + 29u) & 0x3fu));
}

int main(int argc, char** argv)
{
    int reps = 100;
    if (argc >= 2)
    {
        reps = atoi(argv[1]);
        if (reps < 1)
            reps = 1;
    }

    unsigned long long pklen = sig_get_pk_len_bytes();
    unsigned long long sklen = sig_get_sk_len_bytes();
    unsigned long long snlen = sig_get_sn_len_bytes();

    unsigned char* pk = (unsigned char*)malloc((size_t)pklen);
    unsigned char* sk = (unsigned char*)malloc((size_t)sklen);
    unsigned char* sn = (unsigned char*)malloc((size_t)snlen);
    if (!pk || !sk || !sn)
    {
        fprintf(stderr, "allocation failed\n");
        free(pk);
        free(sk);
        free(sn);
        return 1;
    }

    unsigned char msg[64];
    fill_message(msg);

    unsigned long long gpk = 0;
    unsigned long long gsk = 0;
    unsigned long long gsn = 0;
    double t_keygen = 0.0;
    double t_sign = 0.0;
    double t_verify = 0.0;
    double c_keygen = 0.0;
    double c_sign = 0.0;
    double c_verify = 0.0;

    for (int i = 0; i < reps; ++i)
    {
        struct timespec t0, t1;
        unsigned long long c0, c1;

        clock_gettime(CLOCK_MONOTONIC, &t0);
        c0 = read_cycles();
        if (sig_keygen(pk, &gpk, sk, &gsk) != 0)
        {
            fprintf(stderr, "keygen failed\n");
            free(pk);
            free(sk);
            free(sn);
            return 1;
        }
        c1 = read_cycles();
        clock_gettime(CLOCK_MONOTONIC, &t1);
        t_keygen += seconds_between(t0, t1);
        c_keygen += (double)(c1 - c0);

        clock_gettime(CLOCK_MONOTONIC, &t0);
        c0 = read_cycles();
        if (sig_sign(sk, gsk, msg, sizeof(msg), sn, &gsn) != 0)
        {
            fprintf(stderr, "sign failed\n");
            free(pk);
            free(sk);
            free(sn);
            return 1;
        }
        c1 = read_cycles();
        clock_gettime(CLOCK_MONOTONIC, &t1);
        t_sign += seconds_between(t0, t1);
        c_sign += (double)(c1 - c0);

        clock_gettime(CLOCK_MONOTONIC, &t0);
        c0 = read_cycles();
        if (sig_verify(pk, gpk, sn, gsn, msg, sizeof(msg)) != 0)
        {
            fprintf(stderr, "verify failed\n");
            free(pk);
            free(sk);
            free(sn);
            return 1;
        }
        c1 = read_cycles();
        clock_gettime(CLOCK_MONOTONIC, &t1);
        t_verify += seconds_between(t0, t1);
        c_verify += (double)(c1 - c0);
    }

    printf("instance=%s\n", ALGORITHM_INSTANCE);
    printf("reps=%d msg_len=64 pk=%llu sk=%llu sig=%llu peak_rss_kb=%ld\n",
           reps, gpk, gsk, gsn, peak_rss_kb());
    printf("operation,avg_ms,avg_cycles,ops_per_second\n");
    printf("keygen,%.6f,%.0f,%.6f\n",
           1000.0 * t_keygen / reps, c_keygen / reps,
           t_keygen > 0.0 ? (double)reps / t_keygen : 0.0);
    printf("sign,%.6f,%.0f,%.6f\n",
           1000.0 * t_sign / reps, c_sign / reps,
           t_sign > 0.0 ? (double)reps / t_sign : 0.0);
    printf("verify,%.6f,%.0f,%.6f\n",
           1000.0 * t_verify / reps, c_verify / reps,
           t_verify > 0.0 ? (double)reps / t_verify : 0.0);

    free(pk);
    free(sk);
    free(sn);
    return 0;
}
