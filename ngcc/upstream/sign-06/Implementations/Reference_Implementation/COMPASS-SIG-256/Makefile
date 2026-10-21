CC ?= /usr/bin/cc
CFLAGS += -Wall -Wextra -Wpedantic -Wmissing-prototypes -Wredundant-decls \
  -Wshadow -Wvla -Wpointer-arith -O3 -fomit-frame-pointer

# KAT compilation flags
KATFLAGS += -Wno-unused-result -O3 -fomit-frame-pointer

# Common sources and headers for KAT generation (no randombytes)
SOURCES = sign.c packing.c polyvec.c poly.c ntt.c reduce.c rounding.c \
          symmetric-shake.c drng.c auxfunc.c
HEADERS = config.h params.h api.h sign.h packing.h polyvec.h poly.h ntt.h \
          reduce.h rounding.h symmetric.h drng.h auxfunc.h

# Test programs additionally need OS-entropy randombytes
TEST_SOURCES = $(SOURCES) randombytes.c
TEST_HEADERS = $(HEADERS) randombytes.h

# KAT generation sources
KAT_SOURCES = KAT_SIG.c SIG_AlgorithmInstance.c
KAT_HEADERS = SIG_AlgorithmInstance.h

.PHONY: all speed shared clean kat

all: \
  test/test_COMPASS_SIG_256

# KAT target
kat: \
  test/test_kat256

speed: \
  test/test_mul \
  test/test_speed256 \

shared: \
  libpqcrystals_COMPASS_SIG256_ref.so \

# Shared library
libpqcrystals_COMPASS_SIG256_ref.so: $(SOURCES) $(HEADERS)
	$(CC) -shared -fPIC $(CFLAGS) -DCOMPASS_SIG_MODE=256 -o $@ $(SOURCES)

# Correctness test (needs OS-entropy randombytes for test data)
test/test_COMPASS_SIG_256: test/test_COMPASS_SIG.c $(TEST_SOURCES) $(TEST_HEADERS)
	$(CC) $(CFLAGS) -DCOMPASS_SIG_MODE=256 -o $@ $< $(TEST_SOURCES)

# Speed benchmark
test/test_speed256: test/test_speed.c test/speed_print.c test/speed_print.h \
  test/cpucycles.c test/cpucycles.h $(TEST_SOURCES) $(TEST_HEADERS)
	$(CC) $(CFLAGS) -DCOMPASS_SIG_MODE=256 \
	  -o $@ $< test/speed_print.c test/cpucycles.c $(TEST_SOURCES)

# NTT/MUL test
test/test_mul: test/test_mul.c $(TEST_SOURCES) $(TEST_HEADERS)
	$(CC) $(CFLAGS) -DCOMPASS_SIG_MODE=256 -o $@ $< $(TEST_SOURCES)

# NGCC KAT generation (deterministic, uses DRNG, no randombytes)
test/test_kat256: $(KAT_SOURCES) $(KAT_HEADERS) $(SOURCES) $(HEADERS)
	$(CC) $(KATFLAGS) -DCOMPASS_SIG_MODE=256 -o $@ $(KAT_SOURCES) $(SOURCES)


clean:
	rm -f *~ test/*~ *.gcno *.gcda *.lcov
	rm -f libpqcrystals_COMPASS_SIG256_ref.so
	rm -f test/test_COMPASS_SIG_*
	rm -f test/test_speed*
	rm -f test/test_mul
	rm -f test/test_kat*
	rm -rf output/
