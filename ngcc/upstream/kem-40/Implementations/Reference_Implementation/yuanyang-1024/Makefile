CC ?= gcc
CFLAGS ?= -std=c99 -Wpedantic -Wall -Wextra -Ofast -fopenmp
LDLIBS ?= -lm
BUILD_DIR ?= build
KAT_BUILD_DIR ?= build-ngcc-kats
INCLUDES = -I. -Ikeygen/ntrugen/src
BENCH_RUNS ?= 1000
BENCH_BUILD_DIR ?= build-benchmark
BENCH_RESULTS_DIR ?= benchmark-results
BENCH_CPU ?= 1
BENCH_RUNNER ?= taskset -c $(BENCH_CPU)
BENCH_RESULT_STEM ?= reference-$(notdir $(CURDIR))
BENCH_CSV ?= $(BENCH_RESULTS_DIR)/$(BENCH_RESULT_STEM).csv
BENCH_LOG ?= $(BENCH_RESULTS_DIR)/$(BENCH_RESULT_STEM).log
BENCH_BUILD_LOG ?= $(BENCH_RESULTS_DIR)/$(BENCH_RESULT_STEM)-build.log
BATCH_BUILD_LOG ?= $(BENCH_RESULTS_DIR)/$(BENCH_RESULT_STEM)-batch-build.log
PROFILE_SIGN_RUNS ?= 1000
PROFILE_KEYGEN_RUNS ?= 100
BATCH_BENCH_RUNS ?= $(PROFILE_SIGN_RUNS)
BATCH_PRESAMPLE_DEPTH ?= 256
TEST_DIR = test

# Code shared by SIG and KEM.
COMMON_SRC = prng.c auxfunc.c drng.c ntt.c codec.c poly_inv_ntt.c
KEM_COMMON_SRC = $(COMMON_SRC)

# Signature implementation and submission utilities.
SIGN_IMPL_SRC = sign.c
SIGN_IMPL_EXTRA_SRC = sign_ntt.c

SIG_SRC = SIG_AlgorithmInstance.c $(SIGN_IMPL_SRC) $(SIGN_IMPL_EXTRA_SRC) \
	vrfy.c sampler.c keygen/keygen.c keygen/pairgen.c keygen/perturbation.c \
	keygen/fft.c keygen/security_loss.c
SIG_NTRUGEN_SRC = keygen/ntrugen/src/ntrugen.c \
	keygen/ntrugen/src/bigint.c keygen/ntrugen/src/bigintgcd.c \
	keygen/ntrugen/src/cfft.c keygen/ntrugen/src/complex.c \
	keygen/ntrugen/src/fft.c keygen/ntrugen/src/twiddles9.c
SIG_CORE_SRC = $(COMMON_SRC) $(SIG_SRC) $(SIG_NTRUGEN_SRC)

# KEM implementation.
KEM_SRC = KEM_AlgorithmInstance.c kem.c kem-keygen.c
KEM_CORE_SRC = $(KEM_COMMON_SRC) $(KEM_SRC)

SIG_CORE_OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SIG_CORE_SRC))
KEM_CORE_OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(KEM_CORE_SRC))
DRNG_ALGORITHM_OBJ = $(BUILD_DIR)/drng_algorithm.o
KAT_SIG_OBJ = $(BUILD_DIR)/KAT_SIG.o
SIG_BENCH_OBJ = $(BUILD_DIR)/benchmark.o
SIG_BATCH_BENCH_OBJ = $(BUILD_DIR)/benchmark_batch.o
KAT_KEM_OBJ = $(BUILD_DIR)/KAT_KEM.o
BENCH_KEM_OBJ = $(BUILD_DIR)/bench-kem.o
CHECK_KAT_SIG_OBJ = $(BUILD_DIR)/check_kat.o
VERIFY_KAT_SIG_OBJ = $(BUILD_DIR)/verify_kat.o
ALL_OBJ = $(sort $(SIG_CORE_OBJ) $(KEM_CORE_OBJ) $(DRNG_ALGORITHM_OBJ) \
	$(KAT_SIG_OBJ) $(SIG_BENCH_OBJ) $(SIG_BATCH_BENCH_OBJ) \
	$(KAT_KEM_OBJ) $(BENCH_KEM_OBJ) \
	$(CHECK_KAT_SIG_OBJ) $(VERIFY_KAT_SIG_OBJ))
DEPS = $(ALL_OBJ:.o=.d)
PROFILE_TEST_TARGETS = sign_bench sign_profiler sign_profiler_internal \
	keygen_profiler sign_batch_bench
BATCH_BENCH_TARGET = SIG_bench_batch

all: kem


kem: KAT_KEM


KAT_KEM: $(KAT_KEM_OBJ) $(KEM_CORE_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

clean:
	rm -rf $(BUILD_DIR) $(KAT_BUILD_DIR) $(BENCH_BUILD_DIR) \
		build-self-assessment \
		$(BENCH_RESULTS_DIR) KAT_SIG SIG_bench SIG_bench_batch benchmark \
		benchmark_sig check_kat verify_kat KAT_KEM BENCH_KEM output
	$(MAKE) -C $(TEST_DIR) clean

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCLUDES) -MMD -MP -c -o $@ $<

-include $(DEPS)

.PHONY: all sig kem benchmark benchmark_batch benchmark_internals \
	bench bench_internal kat_ngcc check clean $(PROFILE_TEST_TARGETS)
