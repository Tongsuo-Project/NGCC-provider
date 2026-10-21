CC ?= cc
CFLAGS ?= -O3 -Wall -Wextra -Wpedantic -fomit-frame-pointer
CPPFLAGS ?= -I. -DLORE_BACKEND_SM3
LDFLAGS ?=

CORE = kem.c pke.c indcpa.c polyvec.c poly.c ntt.c sampler.c reduce.c \
       lore_symmetric.c lore_sm3_wrapper.c lore_frame.c auxfunc.c toomcook.c verify.c \
       bch_codec.c randombytes.c
KAT = KAT_KEM.c KEM_AlgorithmInstance.c drng.c
HEADERS = $(wildcard *.h test/*.h)

.PHONY: all kat_api test check speed clean

all: kat_api
kat_api: KAT_KEM_1 KAT_KEM_2 KAT_KEM_3 KAT_KEM_4
test: test_lore_1 test_lore_2 test_lore_3 test_lore_4
speed: test_speed_1 test_speed_2 test_speed_3 test_speed_4

KAT_KEM_%: $(CORE) $(KAT) $(HEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DLORE_LEVEL=$* $(filter %.c,$^) -o $@ $(LDFLAGS)

test_lore_%: $(CORE) test/test_lore.c $(HEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DLORE_LEVEL=$* $(filter %.c,$^) -o $@ $(LDFLAGS)

test_speed_%: $(CORE) test/test_speed.c test/cpucycles.c test/speed_print.c $(HEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DLORE_LEVEL=$* $(filter %.c,$^) -o $@ $(LDFLAGS)

test_l4_new: $(CORE) test/test_l4_new.c $(HEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DLORE_LEVEL=4 $(filter %.c,$^) -o $@ $(LDFLAGS)

test_sampler: $(CORE) test/test_sampler.c $(HEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DLORE_LEVEL=4 $(filter %.c,$^) -o $@ $(LDFLAGS)

test_api_contract: $(CORE) test/test_api_contract.c $(HEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DLORE_LEVEL=4 $(filter %.c,$^) -o $@ $(LDFLAGS)

test_call_layer: $(CORE) test/test_call_layer.c $(HEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DLORE_LEVEL=4 $(filter %.c,$^) -o $@ $(LDFLAGS)

test_bch_encoding: $(CORE) test/test_bch_encoding.c $(HEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DLORE_LEVEL=4 $(filter %.c,$^) -o $@ $(LDFLAGS)

test_threads: $(CORE) test/test_threads.c $(HEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DLORE_LEVEL=4 $(filter %.c,$^) -o $@ $(LDFLAGS) -pthread

check: test test_l4_new test_sampler test_api_contract test_bch_encoding test_threads test_call_layer
	./test_lore_1
	./test_lore_2
	./test_lore_3
	./test_lore_4
	./test_l4_new
	./test_sampler
	./test_api_contract
	./test_bch_encoding
	./test_threads
	./test_call_layer

clean:
	rm -f KAT_KEM_[1-4] test_lore_[1-4] test_speed_[1-4]
	rm -f test_l4_new test_sampler test_api_contract test_bch_encoding test_threads
