CC      ?= gcc
CFLAGS  ?= -O2 -std=c99 -Wall -Wextra -Wpedantic -Wno-unused-parameter -I . -I src

NTRE_SRC = src/poly.c src/ntt.c src/symmetric.c KEM_AlgorithmInstance.c
NGCC_SRC = auxfunc.c drng.c
KAT_SRC  = KAT_KEM.c

.PHONY: all clean test

all: bin/KAT_KEM

bin/KAT_KEM: $(NTRE_SRC) $(NGCC_SRC) $(KAT_SRC)
	mkdir -p bin
	$(CC) $(CFLAGS) -o $@ $(NTRE_SRC) $(NGCC_SRC) $(KAT_SRC)

test:
	$(MAKE) -C test

clean:
	rm -rf bin
	$(MAKE) -C test clean
