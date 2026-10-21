CC = gcc
CFLAGS ?= -std=c99 -O2 -Wall -Wextra -pedantic
SAN_CFLAGS ?= -std=c99 -O1 -g -Wall -Wextra -pedantic -fsanitize=address,undefined -fno-omit-frame-pointer

CORE_OBJS = KEM_AlgorithmInstance.o nss_hqc_core.o code_layer.o auxfunc.o drng.o
KAT_OBJS = KAT_KEM.o $(CORE_OBJS)
SELFTEST_OBJS = selftest.o nss_hqc_core.o code_layer.o auxfunc.o drng.o
HARDENING_OBJS = hardening_test.o nss_hqc_core.o code_layer.o auxfunc.o drng.o

all: kat_kem

kat_kem: $(KAT_OBJS)
	$(CC) $(CFLAGS) -o $@ $(KAT_OBJS)

selftest: $(SELFTEST_OBJS)
	$(CC) $(CFLAGS) -o $@ $(SELFTEST_OBJS)
	./selftest

hardening_test: $(HARDENING_OBJS)
	$(CC) $(CFLAGS) -o $@ $(HARDENING_OBJS)

hardening: hardening_test
	./hardening_test

sanitize:
	$(MAKE) clean
	$(MAKE) selftest CFLAGS="$(SAN_CFLAGS)"
	$(MAKE) hardening CFLAGS="$(SAN_CFLAGS)"

kat: kat_kem
	./kat_kem

test:
	$(MAKE) selftest
	$(MAKE) kat

clean:
	rm -f *.o kat_kem selftest hardening_test
	rm -rf output