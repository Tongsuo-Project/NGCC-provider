.POSIX:

CC = gcc
CFLAGS = -D_GNU_SOURCE -std=c99 -Wpedantic -Wall -Wextra -O2 -w
LD = $(CC)
LIBS = -lquadmath -lgmp -lm

UNAME_S = $(shell uname -s 2>/dev/null)
ifneq (,$(filter MINGW% MSYS% CYGWIN%,$(UNAME_S)))
LDFLAGS = -Wl,--stack=4194304
PSAPI_LIBS = -lpsapi
else ifeq ($(OS),Windows_NT)
LDFLAGS = -Wl,--stack=4194304
PSAPI_LIBS = -lpsapi
else
LDFLAGS =
PSAPI_LIBS =
endif

# Object files for CTL-257-512
CORE_OBJ = api_257_512.o \
	codec.o fft.o fnr.o kem257.o kem769.o kem3329.o \
	ntru_utils.o ng_fxp.o ng_mp31.o ng_ntru.o ng_poly.o ng_zint31.o \
	sha3.o modqp.o prng.o auxfunc.o drng.o ntru_solver.o keygen.o

OBJ = $(CORE_OBJ)

BINS = test_kem KAT_KEM benchmark test_kem_correctness_ctl benchmark_selfeval_ctl

all: test_kem KAT_KEM benchmark test_kem_correctness_ctl benchmark_selfeval_ctl

test_kem: test_kem.o KEM_AlgorithmInstance.o $(OBJ)
	$(LD) $(LDFLAGS) -o test_kem test_kem.o KEM_AlgorithmInstance.o $(OBJ) $(LIBS)

KAT_KEM: KAT_KEM.o KEM_AlgorithmInstance.o $(OBJ)
	$(LD) $(LDFLAGS) -o KAT_KEM KAT_KEM.o KEM_AlgorithmInstance.o $(OBJ) $(LIBS)

benchmark: benchmark.o KEM_AlgorithmInstance.o $(OBJ)
	$(LD) $(LDFLAGS) -o benchmark benchmark.o KEM_AlgorithmInstance.o $(OBJ) $(LIBS)

test_kem_correctness_ctl: test_kem_correctness_ctl.o KEM_AlgorithmInstance.o $(OBJ)
	$(LD) $(LDFLAGS) -o test_kem_correctness_ctl test_kem_correctness_ctl.o KEM_AlgorithmInstance.o $(OBJ) $(LIBS)

benchmark_selfeval_ctl: benchmark_selfeval_ctl.o KEM_AlgorithmInstance.o $(OBJ)
	$(LD) $(LDFLAGS) -o benchmark_selfeval_ctl benchmark_selfeval_ctl.o KEM_AlgorithmInstance.o $(OBJ) $(LIBS) $(PSAPI_LIBS)

clean:
	-rm -f *.o *.exe $(BINS)

# Core library targets
api_257_512.o: api_257_512.c api.c ctl.h inner.h auxfunc.h
	$(CC) $(CFLAGS) -c -o api_257_512.o api_257_512.c

codec.o: codec.c inner.h
	$(CC) $(CFLAGS) -c -o codec.o codec.c

fft.o: fft.c inner.h
	$(CC) $(CFLAGS) -c -o fft.o fft.c

fnr.o: fnr.c inner.h
	$(CC) $(CFLAGS) -c -o fnr.o fnr.c

kem257.o: kem257.c modgen.c inner.h
	$(CC) $(CFLAGS) -c -o kem257.o kem257.c

kem769.o: kem769.c modgen.c inner.h
	$(CC) $(CFLAGS) -c -o kem769.o kem769.c

kem3329.o: kem3329.c modgen.c inner.h
	$(CC) $(CFLAGS) -c -o kem3329.o kem3329.c

keygen.o: keygen.c keygen_primes_4096.h inner.h ntru_utils.h ntru_solver.h
	$(CC) $(CFLAGS) -c -o keygen.o keygen.c

ntru_solver.o: ntru_solver.c ntru_solver.h
	$(CC) $(CFLAGS) -c -o ntru_solver.o ntru_solver.c

ntru_utils.o: ntru_utils.c ntru_utils.h ng_inner.h ng_config.h sha3.h
	$(CC) $(CFLAGS) -c -o ntru_utils.o ntru_utils.c

ng_fxp.o: ng_fxp.c ng_inner.h ng_config.h sha3.h
	$(CC) $(CFLAGS) -DNTRUGEN_PREFIX=ctlng -c -o ng_fxp.o ng_fxp.c

ng_mp31.o: ng_mp31.c ng_inner.h ng_config.h sha3.h
	$(CC) $(CFLAGS) -DNTRUGEN_PREFIX=ctlng -c -o ng_mp31.o ng_mp31.c

ng_ntru.o: ng_ntru.c ng_inner.h ng_config.h sha3.h
	$(CC) $(CFLAGS) -DNTRUGEN_PREFIX=ctlng -c -o ng_ntru.o ng_ntru.c

ng_poly.o: ng_poly.c ng_inner.h ng_config.h sha3.h
	$(CC) $(CFLAGS) -DNTRUGEN_PREFIX=ctlng -c -o ng_poly.o ng_poly.c

ng_zint31.o: ng_zint31.c ng_inner.h ng_config.h sha3.h
	$(CC) $(CFLAGS) -DNTRUGEN_PREFIX=ctlng -c -o ng_zint31.o ng_zint31.c

sha3.o: sha3.c ng_inner.h ng_config.h sha3.h auxfunc.h
	$(CC) $(CFLAGS) -DNTRUGEN_PREFIX=ctlng -c -o sha3.o sha3.c

modqp.o: modqp.c modgen.c inner.h
	$(CC) $(CFLAGS) -c -o modqp.o modqp.c

prng.o: prng.c inner.h
	$(CC) $(CFLAGS) -c -o prng.o prng.c

auxfunc.o: auxfunc.c auxfunc.h
	$(CC) $(CFLAGS) -c -o auxfunc.o auxfunc.c

drng.o: drng.c drng.h
	$(CC) $(CFLAGS) -c -o drng.o drng.c

# KEM targets
KEM_AlgorithmInstance.o: KEM_AlgorithmInstance.c KEM_AlgorithmInstance.h ctl.h drng.h
	$(CC) $(CFLAGS) -c -o KEM_AlgorithmInstance.o KEM_AlgorithmInstance.c

test_kem.o: test_kem.c KEM_AlgorithmInstance.h drng.h
	$(CC) $(CFLAGS) -c -o test_kem.o test_kem.c

benchmark.o: benchmark.c KEM_AlgorithmInstance.h drng.h
	$(CC) $(CFLAGS) -c -o benchmark.o benchmark.c

test_kem_correctness_ctl.o: test_kem_correctness_ctl.c KEM_AlgorithmInstance.h drng.h ctl.h
	$(CC) $(CFLAGS) -c -o test_kem_correctness_ctl.o test_kem_correctness_ctl.c

benchmark_selfeval_ctl.o: benchmark_selfeval_ctl.c KEM_AlgorithmInstance.h drng.h ctl.h
	$(CC) $(CFLAGS) -c -o benchmark_selfeval_ctl.o benchmark_selfeval_ctl.c

KAT_KEM.o: KAT_KEM.c KEM_AlgorithmInstance.h drng.h
	$(CC) $(CFLAGS) -c -o KAT_KEM.o KAT_KEM.c

.PHONY: all clean
