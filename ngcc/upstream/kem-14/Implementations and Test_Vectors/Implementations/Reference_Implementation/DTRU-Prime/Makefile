CC ?= gcc
CFLAGS += -std=c99 -Wpedantic -Wall -Wextra -O2

SOURCES = dtru.c poly.c reduce.c coding.c  cbd.c radix_ntt_n1087.c inverse.c\
  pack.c KEM_AlgorithmInstance.c auxfunc.c drng.c
HEADERS = params.h dtru.h poly.h reduce.h coding.h radix_ntt_n1087.h inverse.h\
  pack.h KEM_AlgorithmInstance.h cbd.h auxfunc.h drng.h

all: KAT_KEM

test: test_correct test_speed test_bw test_storage


KAT_KEM: KAT_KEM.c $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $< $(SOURCES) -o $@ $(LDFLAGS)

test_correct: test_correctness.c $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $< $(SOURCES) -o $@ $(LDFLAGS)

test_speed: test_speed.c $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $< $(SOURCES) -o $@ $(LDFLAGS)

test_bw: test_bw.c $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $< $(SOURCES) -o $@ $(LDFLAGS)

test_storage: test_storage.c $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $< $(SOURCES) -o $@ $(LDFLAGS)

.PHONY: clean

clean:
	rm -f KAT_KEM
	rm -f test_correct
	rm -f test_speed
	rm -f test_bw
	rm -f test_storage
	rm -rf output
