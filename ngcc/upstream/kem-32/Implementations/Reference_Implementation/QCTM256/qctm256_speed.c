#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#if defined(__APPLE__)
#include <stdint.h>
#include <sys/sysctl.h>
#elif defined(__linux__)
#include <ctype.h>
#endif

#include "rng.h"
#include "scheme_api.h"

#define DEFAULT_KEYGEN_ITERS 0UL
#define DEFAULT_ENCAPS_ITERS 100UL
#define DEFAULT_DECAPS_ITERS 100UL
#define DEFAULT_CPU_MHZ 3000.0

typedef struct {
    unsigned long iterations;
    double total_ns;
} bench_result_t;

static void usage(const char *prog)
{
    fprintf(stderr, "usage: %s [keygen_iters [encaps_iters [decaps_iters]]]\n",
            prog);
    fprintf(stderr, "       iteration count 0 skips that operation\n");
    fprintf(stderr, "       skipped keygen tries the first KAT vector, then synthetic setup\n");
    fprintf(stderr, "       SPEED_CPU_MHZ=<mhz> overrides kcycles estimation\n");
}

static int parse_ulong_arg(const char *text, unsigned long *value)
{
    char *end = NULL;

    errno = 0;
    *value = strtoul(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || *value == ULONG_MAX) {
        return -1;
    }
    return 0;
}

static int hex_value(int ch)
{
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    if (ch >= 'a' && ch <= 'f') {
        return ch - 'a' + 10;
    }
    if (ch >= 'A' && ch <= 'F') {
        return ch - 'A' + 10;
    }
    return -1;
}

static int decode_hex_line(const char *hex, unsigned char *out, size_t out_len)
{
    size_t i;

    for (i = 0; i < out_len; i++) {
        int hi = hex_value((unsigned char)hex[2U * i]);
        int lo = hex_value((unsigned char)hex[2U * i + 1U]);

        if (hi < 0 || lo < 0) {
            return -1;
        }
        out[i] = (unsigned char)((hi << 4) | lo);
    }

    if (hex[2U * out_len] != '\0' &&
        hex[2U * out_len] != '\n' &&
        hex[2U * out_len] != '\r') {
        return -1;
    }
    return 0;
}

static int load_first_kat_vector_from_file(const char *path,
                                           unsigned char *pk,
                                           unsigned char *sk,
                                           unsigned char *ct,
                                           unsigned char *ss)
{
    FILE *fp = fopen(path, "r");
    char *line = NULL;
    size_t cap = 0;
    int have_pk = 0;
    int have_sk = 0;
    int have_ct = 0;
    int have_ss = 0;

    if (fp == NULL) {
        return -1;
    }

    while (getline(&line, &cap, fp) >= 0) {
        if (strncmp(line, "pk = ", 5) == 0 ||
            strncmp(line, "PK = ", 5) == 0) {
            have_pk = decode_hex_line(line + 5, pk, CRYPTO_PUBLICKEYBYTES) == 0;
        } else if (strncmp(line, "sk = ", 5) == 0 ||
                   strncmp(line, "SK = ", 5) == 0) {
            have_sk = decode_hex_line(line + 5, sk, CRYPTO_SECRETKEYBYTES) == 0;
        } else if (strncmp(line, "ct = ", 5) == 0 ||
                   strncmp(line, "CT = ", 5) == 0) {
            have_ct = decode_hex_line(line + 5, ct, CRYPTO_CIPHERTEXTBYTES) == 0;
        } else if (strncmp(line, "ss = ", 5) == 0 ||
                   strncmp(line, "SS = ", 5) == 0) {
            have_ss = decode_hex_line(line + 5, ss, CRYPTO_BYTES) == 0;
        }

        if (have_pk && have_sk && have_ct && have_ss) {
            free(line);
            fclose(fp);
            return 0;
        }
    }

    free(line);
    fclose(fp);
    return -1;
}

static int load_first_kat_vector(unsigned char *pk,
                                 unsigned char *sk,
                                 unsigned char *ct,
                                 unsigned char *ss,
                                 char *source,
                                 size_t source_len)
{
    const char *prefixes[] = {
        "output/",
        "../../../Test_Vectors/",
        ""
    };
    char crypto_kat_file[256];
    size_t i;

    for (i = 0; i < sizeof(prefixes) / sizeof(prefixes[0]); i++) {
        snprintf(crypto_kat_file, sizeof(crypto_kat_file), "%sKAT_KEM_%s.txt",
                 prefixes[i], CRYPTO_ALGNAME);
        if (load_first_kat_vector_from_file(crypto_kat_file, pk, sk, ct, ss) == 0) {
            snprintf(source, source_len, "%s", crypto_kat_file);
            return 0;
        }
    }

    return -1;
}

static double elapsed_ns(const struct timespec *start,
                         const struct timespec *end)
{
    time_t sec = end->tv_sec - start->tv_sec;
    long nsec = end->tv_nsec - start->tv_nsec;

    if (nsec < 0) {
        sec--;
        nsec += 1000000000L;
    }
    return (double)sec * 1000000000.0 + (double)nsec;
}

static int now_monotonic(struct timespec *ts)
{
    return clock_gettime(CLOCK_MONOTONIC, ts);
}

static double parse_cpu_mhz_env(const char **source)
{
    const char *env = getenv("SPEED_CPU_MHZ");
    char *end = NULL;
    double mhz;

    if (env == NULL || *env == '\0') {
        return 0.0;
    }

    errno = 0;
    mhz = strtod(env, &end);
    if (errno != 0 || end == env || *end != '\0' || mhz <= 0.0) {
        return 0.0;
    }

    *source = "SPEED_CPU_MHZ";
    return mhz;
}

#if defined(__APPLE__)
static double detect_cpu_mhz_platform(const char **source)
{
    const char *names[] = {
        "hw.cpufrequency",
        "hw.cpufrequency_max"
    };
    size_t i;

    for (i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
        uint64_t hz = 0;
        size_t len = sizeof(hz);

        if (sysctlbyname(names[i], &hz, &len, NULL, 0) == 0 && hz > 0) {
            *source = names[i];
            return (double)hz / 1000000.0;
        }
    }
    return 0.0;
}
#elif defined(__linux__)
static double detect_cpu_mhz_platform(const char **source)
{
    FILE *fp = fopen("/proc/cpuinfo", "r");
    char line[256];

    if (fp == NULL) {
        return 0.0;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        const char *label = "cpu MHz";
        size_t label_len = strlen(label);

        if (strncmp(line, label, label_len) == 0) {
            char *colon = strchr(line, ':');
            char *end = NULL;
            double mhz;

            if (colon == NULL) {
                continue;
            }
            colon++;
            while (isspace((unsigned char)*colon)) {
                colon++;
            }
            errno = 0;
            mhz = strtod(colon, &end);
            if (errno == 0 && end != colon && mhz > 0.0) {
                fclose(fp);
                *source = "/proc/cpuinfo";
                return mhz;
            }
        }
    }

    fclose(fp);
    return 0.0;
}
#else
static double detect_cpu_mhz_platform(const char **source)
{
    (void)source;
    return 0.0;
}
#endif

static double detect_cpu_mhz(const char **source)
{
    double mhz = parse_cpu_mhz_env(source);

    if (mhz > 0.0) {
        return mhz;
    }

    mhz = detect_cpu_mhz_platform(source);
    if (mhz > 0.0) {
        return mhz;
    }

    *source = "default-assumption";
    return DEFAULT_CPU_MHZ;
}

static void fill_entropy(unsigned char entropy_input[48])
{
    int i;

    for (i = 0; i < 48; i++) {
        entropy_input[i] = (unsigned char)i;
    }
}

static void fill_deterministic(unsigned char *out, size_t len,
                               unsigned int domain)
{
    size_t i;

    for (i = 0; i < len; i++) {
        out[i] = (unsigned char)((i * 131U + domain * 17U) & 0xffU);
    }
}

static int benchmark_keygen(unsigned long iterations,
                            unsigned char *pk,
                            unsigned char *sk,
                            bench_result_t *result)
{
    struct timespec start;
    struct timespec end;
    unsigned long i;

    if (now_monotonic(&start) != 0) {
        perror("clock_gettime");
        return -1;
    }

    for (i = 0; i < iterations; i++) {
        if (crypto_kem_keypair(pk, sk) != SUCCESS) {
            fprintf(stderr, "crypto_kem_keypair failed at iteration %lu\n",
                    i + 1);
            return -1;
        }
    }

    if (now_monotonic(&end) != 0) {
        perror("clock_gettime");
        return -1;
    }

    result->iterations = iterations;
    result->total_ns = elapsed_ns(&start, &end);
    return 0;
}

static int benchmark_encaps(unsigned long iterations,
                            unsigned char *ct,
                            unsigned char *ss,
                            unsigned char *pk,
                            bench_result_t *result)
{
    struct timespec start;
    struct timespec end;
    unsigned long i;

    if (now_monotonic(&start) != 0) {
        perror("clock_gettime");
        return -1;
    }

    for (i = 0; i < iterations; i++) {
        if (crypto_kem_enc(ct, ss, pk) != SUCCESS) {
            fprintf(stderr, "crypto_kem_enc failed at iteration %lu\n", i + 1);
            return -1;
        }
    }

    if (now_monotonic(&end) != 0) {
        perror("clock_gettime");
        return -1;
    }

    result->iterations = iterations;
    result->total_ns = elapsed_ns(&start, &end);
    return 0;
}

static int benchmark_decaps(unsigned long iterations,
                            unsigned char *ss_out,
                            const unsigned char *ss_expected,
                            unsigned char *ct,
                            unsigned char *sk,
                            bench_result_t *result)
{
    struct timespec start;
    struct timespec end;
    unsigned long i;

    if (now_monotonic(&start) != 0) {
        perror("clock_gettime");
        return -1;
    }

    for (i = 0; i < iterations; i++) {
        if (crypto_kem_dec(ss_out, ct, sk) != SUCCESS) {
            fprintf(stderr, "crypto_kem_dec failed at iteration %lu\n", i + 1);
            return -1;
        }
        if (memcmp(ss_out, ss_expected, CRYPTO_BYTES) != 0) {
            fprintf(stderr, "crypto_kem_dec mismatch at iteration %lu\n",
                    i + 1);
            return -1;
        }
    }

    if (now_monotonic(&end) != 0) {
        perror("clock_gettime");
        return -1;
    }

    result->iterations = iterations;
    result->total_ns = elapsed_ns(&start, &end);
    return 0;
}

static void print_result(const char *name,
                         const bench_result_t *result,
                         double cpu_mhz)
{
    if (result->iterations == 0) {
        printf("%-8s %10lu %14s %14s %16s\n",
               name, 0UL, "skipped", "skipped", "skipped");
        return;
    }

    double avg_ns = result->total_ns / (double)result->iterations;
    double avg_ms = avg_ns / 1000000.0;
    double total_ms = result->total_ns / 1000000.0;
    double kcycles = avg_ns * cpu_mhz / 1000000.0;

    printf("%-8s %10lu %14.3f %14.6f %16.3f\n",
           name, result->iterations, total_ms, avg_ms, kcycles);
}

int main(int argc, char **argv)
{
    unsigned long keygen_iters = DEFAULT_KEYGEN_ITERS;
    unsigned long encaps_iters = DEFAULT_ENCAPS_ITERS;
    unsigned long decaps_iters = DEFAULT_DECAPS_ITERS;
    unsigned char entropy_input[48];
    unsigned char *pk = NULL;
    unsigned char *sk = NULL;
    unsigned char *ct = NULL;
    unsigned char *ss = NULL;
    unsigned char *ss_dec = NULL;
    bench_result_t keygen_result = {0UL, 0.0};
    bench_result_t encaps_result = {0UL, 0.0};
    bench_result_t decaps_result = {0UL, 0.0};
    const char *cpu_mhz_source = NULL;
    char setup_source[128] = "measured keygen";
    double cpu_mhz;
    int decaps_arg_provided = argc >= 4;
    int setup_valid_roundtrip = 0;
    int rc = 1;

    if (argc > 4) {
        usage(argv[0]);
        return 1;
    }
    if (argc >= 2 && strcmp(argv[1], "-h") == 0) {
        usage(argv[0]);
        return 0;
    }
    if (argc >= 2 && parse_ulong_arg(argv[1], &keygen_iters) != 0) {
        usage(argv[0]);
        return 1;
    }
    if (argc >= 3 && parse_ulong_arg(argv[2], &encaps_iters) != 0) {
        usage(argv[0]);
        return 1;
    }
    if (argc >= 4 && parse_ulong_arg(argv[3], &decaps_iters) != 0) {
        usage(argv[0]);
        return 1;
    }

    pk = malloc(CRYPTO_PUBLICKEYBYTES);
    sk = malloc(CRYPTO_SECRETKEYBYTES);
    ct = malloc(CRYPTO_CIPHERTEXTBYTES);
    ss = malloc(CRYPTO_BYTES);
    ss_dec = malloc(CRYPTO_BYTES);
    if (pk == NULL || sk == NULL || ct == NULL || ss == NULL ||
        ss_dec == NULL) {
        fprintf(stderr, "allocation failure\n");
        goto cleanup;
    }

    fill_entropy(entropy_input);
    randombytes_init(entropy_input, NULL, 256);

    if (keygen_iters == 0) {
        if (load_first_kat_vector(pk, sk, ct, ss, setup_source,
                                  sizeof(setup_source)) != 0) {
            fill_deterministic(pk, CRYPTO_PUBLICKEYBYTES, 1U);
            fill_deterministic(sk, CRYPTO_SECRETKEYBYTES, 2U);
            fill_deterministic(ct, CRYPTO_CIPHERTEXTBYTES, 3U);
            fill_deterministic(ss, CRYPTO_BYTES, 4U);
            snprintf(setup_source, sizeof(setup_source),
                     "synthetic deterministic buffers");
            if (decaps_iters > 0 && !decaps_arg_provided) {
                fprintf(stderr, "warning: no valid KAT vector for current sizes; ");
                fprintf(stderr, "default dec benchmark skipped\n");
                decaps_iters = 0;
            }
        } else {
            setup_valid_roundtrip = 1;
        }
    } else {
        printf("running keygen benchmark (%lu iterations)\n", keygen_iters);
        fflush(stdout);
        if (benchmark_keygen(keygen_iters, pk, sk, &keygen_result) != 0) {
            goto cleanup;
        }
        setup_valid_roundtrip = 1;
    }

    if (encaps_iters > 0) {
        printf("running enc benchmark (%lu iterations)\n", encaps_iters);
        fflush(stdout);
        if (benchmark_encaps(encaps_iters, ct, ss, pk, &encaps_result) != 0) {
            goto cleanup;
        }
    } else if (keygen_iters > 0 &&
               crypto_kem_enc(ct, ss, pk) != SUCCESS) {
        fprintf(stderr, "setup crypto_kem_enc failed\n");
        goto cleanup;
    }

    if (decaps_iters > 0) {
        if (!setup_valid_roundtrip &&
            crypto_kem_dec(ss, ct, sk) != SUCCESS) {
            fprintf(stderr, "setup crypto_kem_dec failed\n");
            goto cleanup;
        }
        printf("running dec benchmark (%lu iterations)\n", decaps_iters);
        fflush(stdout);
        if (benchmark_decaps(decaps_iters, ss_dec, ss, ct, sk,
                             &decaps_result) != 0) {
            goto cleanup;
        }
    }

    cpu_mhz = detect_cpu_mhz(&cpu_mhz_source);

    printf("%s speed\n", CRYPTO_ALGNAME);
    printf("Parameters: pk=%d bytes sk=%d bytes ct=%d bytes ss=%d bytes\n",
           CRYPTO_PUBLICKEYBYTES, CRYPTO_SECRETKEYBYTES,
           CRYPTO_CIPHERTEXTBYTES, CRYPTO_BYTES);
    printf("Setup key/ciphertext source: %s\n", setup_source);
    printf("Decapsulation check: %s\n",
           setup_valid_roundtrip ? "shared-secret roundtrip" :
           "repeatability only");
    printf("CPU MHz for kcycles estimate: %.3f (%s)\n",
           cpu_mhz, cpu_mhz_source);
    printf("kcycles_est = average wall-time * CPU MHz; not a hardware cycle counter\n");
    printf("\n");
    printf("%-8s %10s %14s %14s %16s\n",
           "op", "iters", "total_ms", "avg_ms", "avg_kcycles_est");
    printf("%-8s %10s %14s %14s %16s\n",
           "--------", "----------", "--------------", "--------------",
           "----------------");
    print_result("keygen", &keygen_result, cpu_mhz);
    print_result("enc", &encaps_result, cpu_mhz);
    print_result("dec", &decaps_result, cpu_mhz);

    rc = 0;

cleanup:
    free(pk);
    free(sk);
    free(ct);
    free(ss);
    free(ss_dec);
    return rc;
}
