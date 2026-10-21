# Build the official API_PKC wrapper, KAT generator and correctness test.
CC ?= cc
ROOT := .
LEVEL := 128
CORE_API := $(ROOT)/Frost/src/api_frost$(LEVEL).h
ARCH_DEFINE ?= _AMD64_
CPPFLAGS := -DNIX -D$(ARCH_DEFINE) -D_REFERENCE_ -D_AES128_FOR_A_ -DFROST_USE_E8_CODE
CFLAGS ?= -std=c99 -Wpedantic -Wall -Wextra -O2
LDLIBS := -lm
ifeq ($(OS),Windows_NT)
STACK_LDFLAGS := -Wl,--stack,67108864
endif

CORE_SOURCES := \
	$(ROOT)/Frost/src/frost$(LEVEL).c \
	$(ROOT)/Frost/src/util.c \
	$(ROOT)/common/sha3/fips202.c \
	$(ROOT)/common/aes/aes_c.c
API_SOURCES := KEM_AlgorithmInstance.c randombytes_adapter.c drng.c auxfunc.c

.PHONY: all kat test smoke check kat-generate kat-install kat-repro clean
all: kat test

kat: KAT_KEM
test: test_kem_api

KAT_KEM: KAT_KEM.c $(API_SOURCES) $(CORE_SOURCES) $(CORE_API)
	$(CC) $(CPPFLAGS) $(CFLAGS) KAT_KEM.c $(API_SOURCES) $(CORE_SOURCES) $(STACK_LDFLAGS) $(LDLIBS) -o $@

test_kem_api: test_kem_api.c $(API_SOURCES) $(CORE_SOURCES) $(CORE_API)
	$(CC) $(CPPFLAGS) $(CFLAGS) test_kem_api.c $(API_SOURCES) $(CORE_SOURCES) $(STACK_LDFLAGS) $(LDLIBS) -o $@

smoke: test_kem_api
	ulimit -s unlimited; ./test_kem_api 5

check: test_kem_api
	ulimit -s unlimited; ./test_kem_api 1000

kat-generate: KAT_KEM
	ulimit -s unlimited; ./KAT_KEM

kat-install: kat-generate
	mkdir -p ../../../Test_Vectors
	cp output/KAT_KEM_MAMBA-Frost-128.txt ../../../Test_Vectors/KAT_KEM_MAMBA-Frost-128.txt

kat-repro: kat-generate
	cmp output/KAT_KEM_MAMBA-Frost-128.txt ../../../Test_Vectors/KAT_KEM_MAMBA-Frost-128.txt

clean:
	rm -rf KAT_KEM test_kem_api output output.first
