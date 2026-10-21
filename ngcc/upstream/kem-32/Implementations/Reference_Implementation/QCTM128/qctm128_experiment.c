#include <errno.h>
#include <float.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <time.h>

#include "scheme_api.h"
#include "seeded_keygen.h"
#include "rng.h"

#define DEFAULT_EXPERIMENTS 5000UL
#define REPORT_FILE "QCTM128_5000_report.md"
#define CSV_FILE "QCTM128_5000_results.csv"

typedef struct {
    unsigned long run_index;
    int status;
    unsigned long attempts_used;
    unsigned long support_rejects;
    unsigned long goppa_poly_rejects;
    unsigned long systematic_form_rejects;
    unsigned long goppa_init_rejects;
    double elapsed_ms;
} experiment_row_t;

static int cmp_ulong(const void *lhs, const void *rhs)
{
    const unsigned long *a = (const unsigned long *)lhs;
    const unsigned long *b = (const unsigned long *)rhs;

    if (*a < *b) {
        return -1;
    }
    if (*a > *b) {
        return 1;
    }
    return 0;
}

static int cmp_double(const void *lhs, const void *rhs)
{
    const double *a = (const double *)lhs;
    const double *b = (const double *)rhs;

    if (*a < *b) {
        return -1;
    }
    if (*a > *b) {
        return 1;
    }
    return 0;
}

static double elapsed_ms(const struct timeval *start,
                         const struct timeval *end)
{
    time_t sec = end->tv_sec - start->tv_sec;
    suseconds_t usec = end->tv_usec - start->tv_usec;

    if (usec < 0) {
        sec--;
        usec += 1000000;
    }
    return (double)sec * 1000.0 + (double)usec / 1000.0;
}

static double percentile_double(double *values, size_t len, double fraction)
{
    size_t index;

    if (len == 0) {
        return 0.0;
    }
    qsort(values, len, sizeof(*values), cmp_double);
    index = (size_t)((double)(len - 1) * fraction + 0.5);
    return values[index];
}

static unsigned long percentile_ulong(unsigned long *values, size_t len,
                                      double fraction)
{
    size_t index;

    if (len == 0) {
        return 0;
    }
    qsort(values, len, sizeof(*values), cmp_ulong);
    index = (size_t)((double)(len - 1) * fraction + 0.5);
    return values[index];
}

static void fill_entropy(unsigned char entropy_input[48])
{
    int i;

    for (i = 0; i < 48; i++) {
        entropy_input[i] = (unsigned char)i;
    }
}

static int write_csv(const experiment_row_t *rows, size_t count)
{
    FILE *fp = fopen(CSV_FILE, "w");
    size_t i;

    if (fp == NULL) {
        return -1;
    }

    fprintf(fp, "run,status,attempts_used,support_rejects,");
    fprintf(fp, "goppa_poly_rejects,systematic_form_rejects,");
    fprintf(fp, "goppa_init_rejects,elapsed_ms\n");

    for (i = 0; i < count; i++) {
        fprintf(fp, "%lu,%d,%lu,%lu,%lu,%lu,%lu,%.3f\n",
                rows[i].run_index, rows[i].status, rows[i].attempts_used,
                rows[i].support_rejects, rows[i].goppa_poly_rejects,
                rows[i].systematic_form_rejects, rows[i].goppa_init_rejects,
                rows[i].elapsed_ms);
    }

    fclose(fp);
    return 0;
}

static int write_report(unsigned long experiments,
                        unsigned long successes,
                        unsigned long failures,
                        unsigned long runs_with_retry,
                        unsigned long total_attempts,
                        unsigned long total_support_rejects,
                        unsigned long total_goppa_poly_rejects,
                        unsigned long total_systematic_form_rejects,
                        unsigned long total_goppa_init_rejects,
                        unsigned long min_attempts,
                        unsigned long median_attempts,
                        unsigned long p95_attempts,
                        unsigned long max_attempts,
                        double total_elapsed_ms,
                        double min_elapsed_ms,
                        double median_elapsed_ms,
                        double p95_elapsed_ms,
                        double max_elapsed_ms)
{
    FILE *fp = fopen(REPORT_FILE, "w");
    struct utsname uts;
    time_t now;
    struct tm local_tm;
    char time_buf[64];

    if (fp == NULL) {
        return -1;
    }

    if (uname(&uts) != 0) {
        memset(&uts, 0, sizeof(uts));
        strcpy(uts.sysname, "unknown");
        strcpy(uts.release, "unknown");
        strcpy(uts.machine, "unknown");
    }

    now = time(NULL);
    localtime_r(&now, &local_tm);
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S %Z", &local_tm);

    fprintf(fp, "# Locally Quasi-Cyclic Twisted McEliece 1 5000-Run Experiment Report\n\n");
    fprintf(fp, "## Experiment Setup\n\n");
    fprintf(fp, "- Generated at: `%s`\n", time_buf);
    fprintf(fp, "- Instance: `QCTM128`\n");
    fprintf(fp, "- Parameters: `(m,n,t,l)=(%d,%d,%d,%d)`\n",
            EXT_DEGREE, LENGTH, NB_ERRORS, ORDER);
    fprintf(fp, "- Experiments: `%lu` calls to `crypto_kem_keypair()`\n",
            experiments);
    fprintf(fp, "- RNG initialization: deterministic KAT-style seed ");
    fprintf(fp, "(`entropy_input[i]=i`, `randombytes_init(..., NULL, 256)`)\n");
    fprintf(fp, "- Platform: `%s %s %s`\n\n",
            uts.sysname, uts.release, uts.machine);

    fprintf(fp, "## Outcome Summary\n\n");
    fprintf(fp, "- Successful key generations: `%lu/%lu`\n",
            successes, experiments);
    fprintf(fp, "- Failed key generations: `%lu/%lu`\n",
            failures, experiments);
    fprintf(fp, "- Runs that needed more than one internal seeded attempt: `%lu`\n",
            runs_with_retry);
    fprintf(fp, "- Final observation: every successful run eventually obtained a ");
    fprintf(fp, "systematic block-circulant parity-check matrix, because ");
    fprintf(fp, "`goppa_keygen()` returned full rank (`m*t`) before ");
    fprintf(fp, "`crypto_kem_keypair()` completed.\n\n");

    fprintf(fp, "## Attempt Statistics\n\n");
    fprintf(fp, "- Total internal seeded attempts: `%lu`\n", total_attempts);
    fprintf(fp, "- Average internal attempts per experiment: `%.4f`\n",
            experiments == 0 ? 0.0 : (double)total_attempts / (double)experiments);
    fprintf(fp, "- Attempts per experiment (`min / median / p95 / max`): ");
    fprintf(fp, "`%lu / %lu / %lu / %lu`\n",
            min_attempts, median_attempts, p95_attempts, max_attempts);
    fprintf(fp, "- Aggregate support-construction rejects: `%lu`\n",
            total_support_rejects);
    fprintf(fp, "- Aggregate Goppa-polynomial rejects: `%lu`\n",
            total_goppa_poly_rejects);
    fprintf(fp, "- Aggregate systematic-form rejects inside `goppa_keygen()`: `%lu`\n",
            total_systematic_form_rejects);
    fprintf(fp, "- Aggregate `goppa_init()` rejects: `%lu`\n\n",
            total_goppa_init_rejects);

    fprintf(fp, "## Timing Statistics\n\n");
    fprintf(fp, "- Total wall-clock time: `%.3f s`\n", total_elapsed_ms / 1000.0);
    fprintf(fp, "- Average time per experiment: `%.3f ms`\n",
            experiments == 0 ? 0.0 : total_elapsed_ms / (double)experiments);
    fprintf(fp, "- Time per experiment (`min / median / p95 / max`): ");
    fprintf(fp, "`%.3f / %.3f / %.3f / %.3f ms`\n\n",
            min_elapsed_ms, median_elapsed_ms, p95_elapsed_ms, max_elapsed_ms);

    fprintf(fp, "## Output Files\n\n");
    fprintf(fp, "- Summary report: `%s`\n", REPORT_FILE);
    fprintf(fp, "- Raw per-run data: `%s`\n", CSV_FILE);

    fclose(fp);
    return 0;
}

int main(int argc, char **argv)
{
    unsigned long experiments = DEFAULT_EXPERIMENTS;
    unsigned long i;
    unsigned long successes = 0;
    unsigned long failures = 0;
    unsigned long runs_with_retry = 0;
    unsigned long total_attempts = 0;
    unsigned long total_support_rejects = 0;
    unsigned long total_goppa_poly_rejects = 0;
    unsigned long total_systematic_form_rejects = 0;
    unsigned long total_goppa_init_rejects = 0;
    unsigned long min_attempts = ULONG_MAX;
    unsigned long max_attempts = 0;
    unsigned long median_attempts;
    unsigned long p95_attempts;
    double total_elapsed_ms = 0.0;
    double min_elapsed_ms = DBL_MAX;
    double max_elapsed_ms = 0.0;
    double median_elapsed_ms;
    double p95_elapsed_ms;
    unsigned long *attempt_values = NULL;
    double *elapsed_values = NULL;
    experiment_row_t *rows = NULL;
    unsigned char entropy_input[48];
    unsigned char *pk = NULL;
    unsigned char *sk = NULL;

    if (argc >= 2) {
        char *end = NULL;
        errno = 0;
        experiments = strtoul(argv[1], &end, 10);
        if (errno != 0 || end == argv[1] || *end != '\0' || experiments == 0) {
            fprintf(stderr, "invalid experiment count: %s\n", argv[1]);
            return 1;
        }
    }

    rows = calloc((size_t)experiments, sizeof(*rows));
    attempt_values = calloc((size_t)experiments, sizeof(*attempt_values));
    elapsed_values = calloc((size_t)experiments, sizeof(*elapsed_values));
    pk = malloc(CRYPTO_PUBLICKEYBYTES);
    sk = malloc(CRYPTO_SECRETKEYBYTES);
    if (rows == NULL || attempt_values == NULL || elapsed_values == NULL ||
        pk == NULL || sk == NULL) {
        fprintf(stderr, "allocation failure\n");
        free(rows);
        free(attempt_values);
        free(elapsed_values);
        free(pk);
        free(sk);
        return 1;
    }

    fill_entropy(entropy_input);
    randombytes_init(entropy_input, NULL, 256);

    for (i = 0; i < experiments; i++) {
        struct timeval start;
        struct timeval end;
        const scheme_keygen_stats_t *stats;
        int status;
        double run_elapsed_ms;

        if (gettimeofday(&start, NULL) != 0) {
            fprintf(stderr, "gettimeofday start failed\n");
            free(rows);
            free(attempt_values);
            free(elapsed_values);
            free(pk);
            free(sk);
            return 1;
        }

        status = crypto_kem_keypair(pk, sk);

        if (gettimeofday(&end, NULL) != 0) {
            fprintf(stderr, "gettimeofday end failed\n");
            free(rows);
            free(attempt_values);
            free(elapsed_values);
            free(pk);
            free(sk);
            return 1;
        }

        stats = scheme_last_keygen_stats();
        run_elapsed_ms = elapsed_ms(&start, &end);

        rows[i].run_index = i + 1;
        rows[i].status = status;
        rows[i].attempts_used = stats->attempts_used;
        rows[i].support_rejects = stats->support_rejects;
        rows[i].goppa_poly_rejects = stats->goppa_poly_rejects;
        rows[i].systematic_form_rejects = stats->systematic_form_rejects;
        rows[i].goppa_init_rejects = stats->goppa_init_rejects;
        rows[i].elapsed_ms = run_elapsed_ms;

        attempt_values[i] = stats->attempts_used;
        elapsed_values[i] = run_elapsed_ms;

        total_attempts += stats->attempts_used;
        total_support_rejects += stats->support_rejects;
        total_goppa_poly_rejects += stats->goppa_poly_rejects;
        total_systematic_form_rejects += stats->systematic_form_rejects;
        total_goppa_init_rejects += stats->goppa_init_rejects;
        total_elapsed_ms += run_elapsed_ms;

        if (stats->attempts_used < min_attempts) {
            min_attempts = stats->attempts_used;
        }
        if (stats->attempts_used > max_attempts) {
            max_attempts = stats->attempts_used;
        }
        if (run_elapsed_ms < min_elapsed_ms) {
            min_elapsed_ms = run_elapsed_ms;
        }
        if (run_elapsed_ms > max_elapsed_ms) {
            max_elapsed_ms = run_elapsed_ms;
        }
        if (stats->attempts_used > 1) {
            runs_with_retry++;
        }

        if (status == 0) {
            successes++;
        } else {
            failures++;
        }
    }

    median_attempts = percentile_ulong(attempt_values, (size_t)experiments, 0.50);
    p95_attempts = percentile_ulong(attempt_values, (size_t)experiments, 0.95);
    median_elapsed_ms = percentile_double(elapsed_values, (size_t)experiments, 0.50);
    p95_elapsed_ms = percentile_double(elapsed_values, (size_t)experiments, 0.95);

    if (write_csv(rows, (size_t)experiments) != 0) {
        fprintf(stderr, "failed to write %s\n", CSV_FILE);
        free(rows);
        free(attempt_values);
        free(elapsed_values);
        free(pk);
        free(sk);
        return 1;
    }

    if (write_report(experiments,
                     successes,
                     failures,
                     runs_with_retry,
                     total_attempts,
                     total_support_rejects,
                     total_goppa_poly_rejects,
                     total_systematic_form_rejects,
                     total_goppa_init_rejects,
                     min_attempts == ULONG_MAX ? 0 : min_attempts,
                     median_attempts,
                     p95_attempts,
                     max_attempts,
                     total_elapsed_ms,
                     min_elapsed_ms == DBL_MAX ? 0.0 : min_elapsed_ms,
                     median_elapsed_ms,
                     p95_elapsed_ms,
                     max_elapsed_ms) != 0) {
        fprintf(stderr, "failed to write %s\n", REPORT_FILE);
        free(rows);
        free(attempt_values);
        free(elapsed_values);
        free(pk);
        free(sk);
        return 1;
    }

    printf("experiments=%lu success=%lu failure=%lu total_time_s=%.3f\n",
           experiments, successes, failures, total_elapsed_ms / 1000.0);
    printf("attempts min=%lu median=%lu p95=%lu max=%lu avg=%.4f\n",
           min_attempts == ULONG_MAX ? 0 : min_attempts,
           median_attempts,
           p95_attempts,
           max_attempts,
           experiments == 0 ? 0.0 : (double)total_attempts / (double)experiments);
    printf("report=%s csv=%s\n", REPORT_FILE, CSV_FILE);

    free(rows);
    free(attempt_values);
    free(elapsed_values);
    free(pk);
    free(sk);
    return 0;
}
