CC ?= gcc
CFLAGS ?= -std=c99 -Wpedantic -Wall -Wextra -O2
LDLIBS ?= -lm
BUILD_DIR ?= build
KAT_BUILD_DIR ?= build-ngcc-kats
BENCH_BUILD_DIR ?= build-benchmark
BENCH_RESULTS_DIR ?= benchmark-results
BENCH_CPU ?= 1
BENCH_RUNNER ?= taskset -c $(BENCH_CPU)
BENCH_RUNS ?= 1000
BENCH_WARMUP_RUNS ?= 2
BATCH_BENCH_RUNS ?= $(BENCH_RUNS)
BATCH_PRESAMPLE_DEPTH ?= 256
GENERIC_CFLAGS ?= -std=c99 -Wpedantic -Wall -Wextra -O2
INCLUDES = -I. -Ikeygen/ntrugen/src
PARAM_NAME := $(notdir $(CURDIR))

COMMON_SRC = common.c prng.c fpr.c fft.c auxfunc.c drng.c ntt.c codec.c \
	poly_inv_ntt.c
SIG_SRC = SIG_AlgorithmInstance.c sign.c sign_ntt.c vrfy.c sampler.c \
	keygen/keygen.c keygen/pairgen.c keygen/perturbation.c \
	keygen/fft.c keygen/security_loss.c
SIG_NTRUGEN_SRC = keygen/ntrugen/src/ntrugen.c \
	keygen/ntrugen/src/bigint.c keygen/ntrugen/src/bigintgcd.c \
	keygen/ntrugen/src/cfft.c keygen/ntrugen/src/complex.c \
	keygen/ntrugen/src/fft.c keygen/ntrugen/src/twiddles9.c
SIG_CORE_SRC = $(COMMON_SRC) $(SIG_SRC) $(SIG_NTRUGEN_SRC)

SIG_CORE_OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SIG_CORE_SRC))
DRNG_ALGORITHM_OBJ = $(BUILD_DIR)/drng_algorithm.o
KAT_SIG_OBJ = $(BUILD_DIR)/KAT_SIG.o
SIG_BENCH_OBJ = $(BUILD_DIR)/benchmark.o
SIG_BATCH_BENCH_OBJ = $(BUILD_DIR)/benchmark_batch.o
CHECK_KAT_SIG_OBJ = $(BUILD_DIR)/check_kat.o
VERIFY_KAT_SIG_OBJ = $(BUILD_DIR)/verify_kat.o
FUNCTIONAL_TEST_OBJ = $(BUILD_DIR)/functional_test.o
ALL_OBJ = $(sort $(SIG_CORE_OBJ) $(DRNG_ALGORITHM_OBJ) $(KAT_SIG_OBJ) \
	$(SIG_BENCH_OBJ) $(SIG_BATCH_BENCH_OBJ) $(CHECK_KAT_SIG_OBJ) \
	$(VERIFY_KAT_SIG_OBJ) $(FUNCTIONAL_TEST_OBJ))
DEPS = $(ALL_OBJ:.o=.d)

all: KAT_SIG SIG_bench SIG_bench_batch check_kat verify_kat functional_test

KAT_SIG: $(KAT_SIG_OBJ) $(SIG_CORE_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

SIG_bench: $(SIG_BENCH_OBJ) $(SIG_CORE_OBJ) $(DRNG_ALGORITHM_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

SIG_bench_batch: $(SIG_BATCH_BENCH_OBJ) $(SIG_CORE_OBJ) $(DRNG_ALGORITHM_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

check_kat: $(CHECK_KAT_SIG_OBJ) $(SIG_CORE_OBJ) $(DRNG_ALGORITHM_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

verify_kat: $(VERIFY_KAT_SIG_OBJ) $(SIG_CORE_OBJ) $(DRNG_ALGORITHM_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

functional_test: $(FUNCTIONAL_TEST_OBJ) $(SIG_CORE_OBJ) $(DRNG_ALGORITHM_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

benchmark:
	@mkdir -p "$(BENCH_RESULTS_DIR)"
	@$(MAKE) -s -B SIG_bench BUILD_DIR="$(BENCH_BUILD_DIR)"
	@$(BENCH_RUNNER) ./SIG_bench $(BENCH_WARMUP_RUNS) >/dev/null 2>&1
	@$(BENCH_RUNNER) ./SIG_bench $(BENCH_RUNS) \
		"$(BENCH_RESULTS_DIR)/reference-$(PARAM_NAME).csv"

benchmark_batch:
	@$(MAKE) -s -B SIG_bench_batch BUILD_DIR="$(BENCH_BUILD_DIR)"
	@$(BENCH_RUNNER) ./SIG_bench_batch $(BENCH_WARMUP_RUNS) \
		$(BATCH_PRESAMPLE_DEPTH) >/dev/null 2>&1
	@$(BENCH_RUNNER) ./SIG_bench_batch $(BATCH_BENCH_RUNS) \
		$(BATCH_PRESAMPLE_DEPTH)

benchmark-generic:
	@$(MAKE) -s -B SIG_bench BUILD_DIR="$(BENCH_BUILD_DIR)-generic" \
		CFLAGS="$(GENERIC_CFLAGS)"
	@./SIG_bench $(BENCH_WARMUP_RUNS) >/dev/null 2>&1
	@./SIG_bench $(BENCH_RUNS)


$(BUILD_DIR)/benchmark.o: benchmark.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCLUDES) \
		-DYUANYANG_BENCH_BUILD_FLAGS='"$(CFLAGS)"' \
		-DYUANYANG_BENCH_BUILD_TOOL='"GNU Make $(MAKE_VERSION)"' \
		-MMD -MP -c -o $@ $<

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCLUDES) -MMD -MP -c -o $@ $<

kat_ngcc:
	$(MAKE) -B KAT_SIG check_kat verify_kat BUILD_DIR="$(KAT_BUILD_DIR)" \
		CFLAGS="$(CFLAGS) -DNGCC_KATS"
	./KAT_SIG
	./check_kat output/KAT_SIG_$(PARAM_NAME).txt
	./verify_kat output/KAT_SIG_$(PARAM_NAME).txt

clean:
	rm -rf $(BUILD_DIR) $(KAT_BUILD_DIR) $(BENCH_BUILD_DIR) \
		$(BENCH_RESULTS_DIR) KAT_SIG SIG_bench SIG_bench_batch \
		check_kat verify_kat functional_test output

-include $(DEPS)

.PHONY: all benchmark benchmark_batch benchmark-generic kat_ngcc clean
