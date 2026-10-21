CC = gcc
CFLAGS = -std=c99 -Wpedantic -Wall -Wextra -O2
LDFLAGS =

# --- Switchable reference backends ---
BIT_USE_SHAKE ?= 0
RL_KEM_USE_CONJ_NTT_REJECTION ?= 0
EXTRA_CFLAGS ?=

CFLAGS += -DBIT_USE_SHAKE=$(BIT_USE_SHAKE)
CFLAGS += -DRL_KEM_USE_CONJ_NTT_REJECTION=$(RL_KEM_USE_CONJ_NTT_REJECTION)
CFLAGS += $(EXTRA_CFLAGS)

COMMON_SOURCES= auxfunc.c drng.c fips202.c symmetric.c sample.c poly.c pke.c polar.c ntt.c fft.c KEM_AlgorithmInstance.c
SOURCES= $(COMMON_SOURCES) test.c
KAT_KEM_SOURCES= $(COMMON_SOURCES) KAT_KEM.c
CYCLES_SOURCES= $(COMMON_SOURCES) cycles_test.c
HEADERS= auxfunc.h drng.h fips202.h symmetric.h sample.h params.h poly.h pke.h polar.h ntt.h fft.h KEM_AlgorithmInstance.h

test: $(HEADERS) $(SOURCES)
	$(CC) $(CFLAGS) -o $@ $(SOURCES) $(LDFLAGS)

kat_kem: $(HEADERS) $(KAT_KEM_SOURCES)
	$(CC) $(CFLAGS) -o $@ $(KAT_KEM_SOURCES) $(LDFLAGS)

cycles_test: $(HEADERS) $(CYCLES_SOURCES)
	$(CC) $(CFLAGS) -o $@ $(CYCLES_SOURCES) $(LDFLAGS)

.PHONY: clean
clean:
	-rm test kat_kem cycles_test
