CC      ?= gcc
CFLAGS  ?= -O2 -std=c99 -Wall -Wextra -Wpedantic -Wno-unused-parameter -I .. -I ../src

NTRE_SRC = ../src/poly.c ../src/ntt.c ../src/symmetric.c ../KEM_AlgorithmInstance.c
NGCC_SRC = ../auxfunc.c ../drng.c
TEST_SRC = test.c

.PHONY: all clean

all: test_ntre

test_ntre: $(NTRE_SRC) $(NGCC_SRC) $(TEST_SRC)
	$(CC) $(CFLAGS) -o $@ $(NTRE_SRC) $(NGCC_SRC) $(TEST_SRC)

clean:
	rm -f test_ntre
