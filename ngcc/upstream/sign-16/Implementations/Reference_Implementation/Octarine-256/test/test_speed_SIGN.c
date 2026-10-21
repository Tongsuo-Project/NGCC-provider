/* [PROFILE] Three end-to-end signature operations: warm up 10 calls, then time 1000 calls.
 * One process per parameter/implementation.
 */
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpucycles.h"
#include "drng.h"
#include "parameters.h"
#include "sign.h"

DRNG_ctx drng_algorithm;
typedef struct { uint64_t start, end, cycles; } sample_record;
static size_t warmup_calls = 10;
static size_t measured_calls = 1000;
static FILE *sample_file;

static size_t parse_count(const char *text, int zero_allowed) {
  char *end;
  unsigned long value = strtoul(text, &end, 10);
  if (!*text || *end || *text == '-' || (!zero_allowed && !value) || value > 1000000)
    arcane_pmu_fail("invalid sample/warmup count");
  return (size_t)value;
}

static int compare_cycles(const void *a, const void *b) {
  uint64_t x = *(const uint64_t *)a, y = *(const uint64_t *)b;
  return (x > y) - (x < y);
}

static void report_operation(const char *name, const sample_record *records,
                             size_t warmups_done, size_t measured_done, uint64_t overhead) {
  if (warmups_done != warmup_calls || measured_done != measured_calls)
    arcane_pmu_fail("operation invocation counts do not match protocol");
  uint64_t *sorted = malloc(measured_calls * sizeof(*sorted));
  if (!sorted) arcane_pmu_fail("allocate median scratch space");
  long double sum = 0;
  for (size_t i = 0; i < measured_calls; i++) {
    sorted[i] = records[i].cycles;
    sum += records[i].cycles;
    if (sample_file)
      fprintf(sample_file, "%s,%zu,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 "\n",
              name, i, records[i].start, records[i].end,
              records[i].end - records[i].start, overhead, records[i].cycles);
  }
  if (sample_file && fflush(sample_file)) arcane_pmu_fail("flush samples");
  qsort(sorted, measured_calls, sizeof(*sorted), compare_cycles);
  double median = measured_calls % 2 ? (double)sorted[measured_calls / 2]
    : ((double)sorted[measured_calls / 2 - 1] + (double)sorted[measured_calls / 2]) / 2.0;
  printf("%s:\nmedian: %.1f CPU cycles\naverage: %.3Lf CPU cycles\n", name, median, sum / measured_calls);
  printf("[operation] name=%s warmups_completed=%zu samples_completed=%zu\n\n",
         name, warmups_done, measured_done);
  free(sorted);
}

/* Keep every call, including verification and warmups, observable under LTO. */
#define MEASURE_OPERATION(NAME, CALL) do { \
  size_t warmed = 0, measured = 0; \
  for (; warmed < warmup_calls; warmed++) { \
    int status = (CALL); \
    __asm__ volatile ("" : "+r"(status) : : "memory"); \
    if (status != 0) arcane_pmu_fail("operation failed during warmup"); \
  } \
  for (; measured < measured_calls; measured++) { \
    uint64_t start = cpucycles(); \
    int status = (CALL); \
    __asm__ volatile ("" : "+r"(status) : : "memory"); \
    uint64_t end = cpucycles(); \
    if (status != 0) arcane_pmu_fail("operation failed during measurement"); \
    records[measured].start = start; \
    records[measured].end = end; \
    records[measured].cycles = cpucycles_elapsed(start, end, overhead); \
  } \
  report_operation(NAME, records, warmed, measured, overhead); \
} while (0)

int main(int argc, char **argv) {
  for (int i = 1; i < argc; i += 2) {
    if (i + 1 >= argc) arcane_pmu_fail("missing argument value");
    if (!strcmp(argv[i], "--warmups")) warmup_calls = parse_count(argv[i + 1], 1);
    else if (!strcmp(argv[i], "--samples")) measured_calls = parse_count(argv[i + 1], 0);
    else arcane_pmu_fail("unknown argument");
  }
  const char *sample_path = getenv("ARCANE_PMU_SAMPLES");
  if (sample_path) {
    sample_file = fopen(sample_path, "wx");
    if (!sample_file) arcane_pmu_fail("sample file exists or cannot be created");
    fprintf(sample_file, "operation,sample,start_cycles,end_cycles,gross_cycles,overhead,cycles\n");
  }
  sample_record *records = calloc(measured_calls, sizeof(*records));
  if (!records) arcane_pmu_fail("allocate sample records");
  unsigned char pk[RRLWR_SIGN_PK_LEN], sk[RRLWR_SIGN_SK_LEN];
  unsigned char sn[RRLWR_SIGN_SIG_LEN], m[32] = {0};
  unsigned long long pk_len = 0, sk_len = 0, sn_len = 0;
  const unsigned char seed[32] = {0};
  if (init_random_number(&drng_algorithm, seed, sizeof(seed)))
    arcane_pmu_fail("initialize benchmark DRNG");
  uint64_t overhead = cpucycles_overhead();
  printf("[protocol] warmup_calls=%zu measured_calls=%zu process_runs=1 unit=PMU_cycles_user_kernel\n",
         warmup_calls, measured_calls);
  MEASURE_OPERATION("sig_keygen", sig_keygen(pk, &pk_len, sk, &sk_len));
  if (pk_len != sizeof(pk) || sk_len != sizeof(sk)) arcane_pmu_fail("signature key lengths");
  MEASURE_OPERATION("sig_sign", sig_sign(sk, sk_len, m, sizeof(m), sn, &sn_len));
  if (sn_len != sizeof(sn)) arcane_pmu_fail("signature length");
  MEASURE_OPERATION("sig_verify", sig_verify(pk, pk_len, sn, sn_len, m, sizeof(m)));
  puts("[correctness] signature_roundtrip=pass");
  /* Validate rejection outside the measured region. */
  m[0] ^= 1;
  if (sig_verify(pk, pk_len, sn, sn_len, m, sizeof(m)) == 0)
    arcane_pmu_fail("modified message accepted");
  puts("[correctness] modified_message_rejected=pass");
  free(records);
  if (sample_file && fclose(sample_file)) arcane_pmu_fail("close samples");
  return 0;
}
