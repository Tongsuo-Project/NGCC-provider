CC ?= gcc
CFLAGS += -std=c99 -Wpedantic -Wall -Wextra -O2

SOURCES = dtru.c poly.c reduce.c coding.c  cbd.c ntt.c inverse.c\
  pack.c KEM_AlgorithmInstance.c auxfunc.c drng.c
HEADERS = params.h dtru.h poly.h reduce.h coding.h ntt.h inverse.h\
  pack.h KEM_AlgorithmInstance.h cbd.h auxfunc.h drng.h

all: KAT_KEM

test: test_correct test_speed test_bw test_storage

test_p: test_correct_p test_speed_p test_bw_p test_storage_p


KAT_KEM: KAT_KEM.c $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $< $(SOURCES) -o $@ $(LDFLAGS)

KAT_KEM_PACK_PK: KAT_KEM.c $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -DPK_PACK_OPT=1 $< $(SOURCES) -o $@ $(LDFLAGS)

test_correct: test_correctness.c $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $< $(SOURCES) -o $@ $(LDFLAGS)

test_correct_p: test_correctness.c $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -DPK_PACK_OPT=1 $< $(SOURCES) -o $@ $(LDFLAGS)

test_speed: test_speed.c $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $< $(SOURCES) -o $@ $(LDFLAGS)

test_speed_p: test_speed.c $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -DPK_PACK_OPT=1 $< $(SOURCES) -o $@ $(LDFLAGS)

test_bw: test_bw.c $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $< $(SOURCES) -o $@ $(LDFLAGS)

test_bw_p: test_bw.c $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -DPK_PACK_OPT=1 $< $(SOURCES) -o $@ $(LDFLAGS)

test_storage: test_storage.c $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $< $(SOURCES) -o $@ $(LDFLAGS)

test_storage_p: test_storage.c $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -DPK_PACK_OPT=1 $< $(SOURCES) -o $@ $(LDFLAGS)

.PHONY: clean

clean:
	rm -f KAT_KEM
	rm -f KAT_KEM_PACK_PK
	rm -f test_correct
	rm -f test_correct_p
	rm -f test_speed
	rm -f test_speed_p
	rm -f test_bw
	rm -f test_bw_p
	rm -f test_storage
	rm -f test_storage_p
	rm -rf output
