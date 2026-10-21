ALGORITHM_INSTANCE = Phoenix-SM3-256s
PARAMS = phoenix-sm3-256s

CC ?= gcc
CFLAGS ?= -O3 -std=c99
CFLAGS += -DPARAMS=$(PARAMS) -I. -DALLOW_DEEP_TREES -DSUBMISSION_DRNG

CORE_SOURCES = \
	address.c \
	auxfunc.c \
	counter.c \
	hash_sm3.c sm3_drng.c \
	merkle.c \
	octopus.c \
	randombytes.c \
	sign.c \
	tfors.c \
	thash_sm3_simple.c \
	utils.c \
	utilsx1.c \
	gwots.c \
	gwotsx1.c

SUBMISSION_SOURCES = \
	SIG_AlgorithmInstance.c \
	drng.c \
	KAT_SIG.c

.PHONY: all clean kat benchmark

all: KAT_SIG

kat: KAT_SIG
	./KAT_SIG

KAT_SIG: $(CORE_SOURCES) $(SUBMISSION_SOURCES) SIG_AlgorithmInstance.h drng.h
	$(CC) $(CFLAGS) -DPARAMNAME=\"$(ALGORITHM_INSTANCE)\" -o $@ $(CORE_SOURCES) $(SUBMISSION_SOURCES)

benchmark: $(CORE_SOURCES) test/benchmark.c test/cycles.c
	$(CC) $(CFLAGS) -U SUBMISSION_DRNG -DPARAMNAME=\"$(ALGORITHM_INSTANCE)\" -o $@ $(CORE_SOURCES) test/benchmark.c test/cycles.c

clean:
	$(RM) KAT_SIG
	$(RM) benchmark
	$(RM) -r output