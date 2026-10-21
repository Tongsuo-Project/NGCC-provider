CC ?= gcc

CFLAGS ?= -O2 -std=c99 -Wpedantic -Wall -Wextra -Wno-missing-braces
CPPFLAGS += -I.
LDFLAGS ?=
LDLIBS += -lcrypto

HASH_BACKEND ?= sm3
ifeq ($(HASH_BACKEND),sm3)
override CPPFLAGS += -DGREATWALL_HASH_BACKEND_SM3
EXTRA_OBJS := auxfunc.o
endif

TARGET := KAT_SIG

SRCS := \
	KAT_SIG.c \
	SIG_AlgorithmInstance.c \
	drng.c \
	api.c \
	aes.c \
	aes_impl.c \
	block.c \
	block_impl.c \
	faest.c \
	greatwall.c \
	greatwall-128-matrix.c \
	greatwall_impl.c \
	hash.c \
	KeccakHash.c \
	KeccakP-1600-reference.c \
	KeccakSponge.c \
	owf_proof.c \
	polynomials.c \
	polynomials_impl.c \
	prgs.c \
	quicksilver.c \
	randomness_randombytes.c \
	small_vole.c \
	transpose.c \
	universal_hash.c \
	util.c \
	vector_com.c \
	vole_check.c \
	vole_commit.c

OBJS := $(SRCS:.c=.o)
DEPS := $(SRCS:.c=.d)

all: $(TARGET)

$(TARGET): $(filter-out randomness_randombytes.o,$(OBJS)) randomness_kat_fixed.o $(EXTRA_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

%.o: %.c Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c -o $@ $<

auxfunc.o: auxfunc.c auxfunc.h Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c -o $@ $<

CLEAN_PROGRAMS := KAT_SIG bench_sig KAT_SIG_sm3_fixed KAT_SIG_deterministic \
	bench_sig_deterministic api_test test_arithmetic test_rng_failure sync_vector

clean:
	rm -f -- .build-config *.o *.d *.a *.so *.so.* \
		$(CLEAN_PROGRAMS) $(addsuffix .exe,$(CLEAN_PROGRAMS))

-include $(DEPS)

.PHONY: all clean

# Rebuild objects when the backend or compiler flags change.
.PHONY: FORCE
FORCE:

.build-config: FORCE
	@printf '%s\n' '$(CC)' '$(CPPFLAGS)' '$(CFLAGS)' '$(LDFLAGS)' '$(LDLIBS)' '$(HASH_BACKEND)' | cmp -s - $@ || \
	  printf '%s\n' '$(CC)' '$(CPPFLAGS)' '$(CFLAGS)' '$(LDFLAGS)' '$(LDLIBS)' '$(HASH_BACKEND)' > $@

$(OBJS) $(EXTRA_OBJS) KAT_SIG.o SIG_AlgorithmInstance.o randomness_kat_fixed.o: .build-config
-include randomness_kat_fixed.d

# Benchmarks use the official DRNG for SM3 and OS randomness for SHAKE.
BENCH_RNG_OBJ := randomness_os.o
ifeq ($(HASH_BACKEND),sm3)
BENCH_RNG_OBJ := randomness_kat_fixed.o
endif
BENCH_OBJS := $(sort $(filter-out KAT_SIG.o randomness_kat_fixed.o randomness_randombytes.o randomness_os.o,$(OBJS)) $(BENCH_RNG_OBJ) SIG_AlgorithmInstance.o $(EXTRA_OBJS))

bench_sig: bench_sig.o $(BENCH_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

bench_sig.o: ../bench_sig.c Makefile .build-config
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c -o $@ $<

randomness_os.o: .build-config

-include bench_sig.d randomness_os.d
