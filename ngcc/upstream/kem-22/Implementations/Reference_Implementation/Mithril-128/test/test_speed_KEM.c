/* [PROFILE] Six end-to-end operations: warm up 10 calls, then time 1000 calls.
 * One process per parameter/implementation. Algorithm implementations are unchanged.
 */
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpucycles.h"
#include "drng.h"
#include "parameters.h"
#include "pke.h"
#include "kem.h"

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
  free(sorted); /* Memory only: no files are removed. */
}

#define MEASURE_OPERATION(NAME, CALL) do { \
  size_t warmed = 0, measured = 0; \
  for (; warmed < warmup_calls; warmed++) { \
    if ((CALL) != 0) arcane_pmu_fail("operation failed during warmup"); \
  } \
  for (; measured < measured_calls; measured++) { \
    uint64_t start = cpucycles(); \
    int status = (CALL); \
    uint64_t end = cpucycles(); \
    if (status != 0) arcane_pmu_fail("operation failed during measurement"); \
    if (end < start || end - start < overhead) arcane_pmu_fail("invalid cycle delta"); \
    records[measured].start = start; \
    records[measured].end = end; \
    records[measured].cycles = end - start - overhead; \
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
  unsigned char seedA[RRLWR_PKE_SEED_A_LEN], seedS[RRLWR_SEED_S_LEN], seedSp[RRLWR_SEED_S_LEN];
  unsigned char pk[RRLWR_PKE_PK_LEN], sk[RRLWR_PKE_SK_LEN], ct[RRLWR_PKE_CT_LEN];
  unsigned char m[RRLWR_PKE_MESSAGE_LEN], mp[RRLWR_PKE_MESSAGE_LEN];
  unsigned char kem_pk[RRLWR_KEM_PK_LEN], kem_sk[RRLWR_KEM_SK_LEN], kem_ct[RRLWR_KEM_CT_LEN];
  unsigned char ss_enc[RRLWR_KEM_SS_LEN], ss_dec[RRLWR_KEM_SS_LEN];
  unsigned long long pk_len = 0, sk_len = 0, ct_len = 0, enc_ss_len = 0, dec_ss_len = 0;
  const unsigned char seed[32] = {0};
  init_random_number(&drng_algorithm, seed, sizeof(seed));
  get_random_number(&drng_algorithm, seedA, 8 * sizeof(seedA));
  get_random_number(&drng_algorithm, seedS, 8 * sizeof(seedS));
  get_random_number(&drng_algorithm, seedSp, 8 * sizeof(seedSp));
  get_random_number(&drng_algorithm, m, 8 * sizeof(m));
  uint64_t overhead = cpucycles_overhead();
  printf("[protocol] warmup_calls=%zu measured_calls=%zu process_runs=1 unit=PMU_cycles_user_kernel\n",
         warmup_calls, measured_calls);
  MEASURE_OPERATION("pke_keygen", pke_keygen(pk, sk, seedA, seedS));
  MEASURE_OPERATION("pke_encrypt", pke_encrypt(ct, pk, m, seedSp));
  MEASURE_OPERATION("pke_decrypt", pke_decrypt(mp, ct, sk));
  if (memcmp(m, mp, sizeof(m))) arcane_pmu_fail("PKE round-trip mismatch");
  puts("[correctness] pke_roundtrip=pass");
  MEASURE_OPERATION("kem_keygen", kem_keygen(kem_pk, &pk_len, kem_sk, &sk_len));
  if (pk_len != sizeof(kem_pk) || sk_len != sizeof(kem_sk)) arcane_pmu_fail("KEM key lengths");
  MEASURE_OPERATION("kem_encaps", kem_enc(kem_pk, pk_len, ss_enc, &enc_ss_len, kem_ct, &ct_len));
  if (enc_ss_len != sizeof(ss_enc) || ct_len != sizeof(kem_ct)) arcane_pmu_fail("KEM encapsulation lengths");
  MEASURE_OPERATION("kem_decaps", kem_dec(kem_sk, sk_len, kem_ct, ct_len, ss_dec, &dec_ss_len));
  if (dec_ss_len != sizeof(ss_dec) || memcmp(ss_enc, ss_dec, sizeof(ss_enc)))
    arcane_pmu_fail("KEM round-trip mismatch");
  puts("[correctness] kem_roundtrip=pass");
  free(records);
  if (sample_file && fclose(sample_file)) arcane_pmu_fail("close samples");
  return 0;
}
