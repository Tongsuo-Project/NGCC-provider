CC = gcc
CFLAGS += -O3
# Keep platform tuning in the optional x86 performance profile.
PERF ?= 0
PERF_CFLAGS ?= -march=x86-64-v3 -mtune=sapphirerapids -mprefer-vector-width=256 -fomit-frame-pointer -fno-stack-protector -DNDEBUG
ifeq ($(PERF),1)
CFLAGS += $(PERF_CFLAGS)
endif
PACKING_FLAGS ?=
TEST_FLAGS = -UNDEBUG
RM = /bin/rm

BASE_FOLDER = .
ARITH_FOLDER = ./arith
UTILS_FOLDER = ./utils
INCLUDES = -I$(BASE_FOLDER) -I$(UTILS_FOLDER) -I$(ARITH_FOLDER)

override SECURITY_LEVEL = 256

SOURCES_RRLWR = $(ARITH_FOLDER)/poly.c $(ARITH_FOLDER)/ring.c $(ARITH_FOLDER)/packing.c $(ARITH_FOLDER)/uniform.c pke.c kem.c
SOURCES = $(SOURCES_RRLWR) $(UTILS_FOLDER)/auxfunc.c
SOURCES_CORRECTNESS = $(ARITH_FOLDER)/poly.c $(ARITH_FOLDER)/ring.c $(ARITH_FOLDER)/packing.c $(ARITH_FOLDER)/uniform.c pke.c $(UTILS_FOLDER)/auxfunc.c
HEADERS_RRLWR = $(ARITH_FOLDER)/poly.h $(ARITH_FOLDER)/ring.h $(ARITH_FOLDER)/packing.h $(ARITH_FOLDER)/uniform.h parameters.h hash_domain.h pke.h kem.h
HEADERS = $(HEADERS_RRLWR) $(UTILS_FOLDER)/auxfunc.h $(UTILS_FOLDER)/drng.h

TEST_TARGET = test/test_KEM$(SECURITY_LEVEL)
UNIT_TEST_TARGET = test/unit_tests_KEM$(SECURITY_LEVEL)
CORRECTNESS_TARGET = test/correctness/arith_correctness$(SECURITY_LEVEL)
SPEED_TARGET = test/test_speed_KEM$(SECURITY_LEVEL)
KAT_TARGET = KAT/KAT_KEM$(SECURITY_LEVEL)
TARGETS = $(TEST_TARGET) $(UNIT_TEST_TARGET) $(CORRECTNESS_TARGET) $(SPEED_TARGET) $(KAT_TARGET)

.PHONY: all test correctness run-correctness speed KAT clean packbit16

all: test speed KAT

packbit16:
	$(MAKE) clean
	$(MAKE) all PACKING_FLAGS=-DRRLWR_PACKING_16BIT_SCALAR=1

test: $(TEST_TARGET) $(UNIT_TEST_TARGET) $(CORRECTNESS_TARGET)
	@if [ "$(run)" = "1" ]; then \
		./$(TEST_TARGET) && \
		./$(UNIT_TEST_TARGET) && \
		./$(CORRECTNESS_TARGET); \
	fi

correctness: $(CORRECTNESS_TARGET)

run-correctness: correctness
	./$(CORRECTNESS_TARGET)

speed: $(SPEED_TARGET)
	@if [ "$(run)" = "1" ]; then \
		./$(SPEED_TARGET); \
	fi

KAT: $(KAT_TARGET)
	@if [ "$(run)" = "1" ]; then \
		./$(KAT_TARGET); \
	fi

$(TEST_TARGET): test/test_KEM%: $(SOURCES) $(HEADERS) $(UTILS_FOLDER)/drng.c $(UTILS_FOLDER)/drng.h test/test_KEM.c
	$(CC) $(INCLUDES) -DRRLWR_SECURITY_LEVEL=$* $(PACKING_FLAGS) $(CFLAGS) $(TEST_FLAGS) $(SOURCES) $(UTILS_FOLDER)/drng.c test/test_KEM.c -o $@

$(UNIT_TEST_TARGET): test/unit_tests_KEM%: $(SOURCES) $(HEADERS) $(UTILS_FOLDER)/drng.c $(UTILS_FOLDER)/drng.h test/unit_tests_KEM.c
	$(CC) $(INCLUDES) -DRRLWR_SECURITY_LEVEL=$* $(PACKING_FLAGS) $(CFLAGS) $(TEST_FLAGS) $(SOURCES) $(UTILS_FOLDER)/drng.c test/unit_tests_KEM.c -o $@

$(CORRECTNESS_TARGET): test/correctness/arith_correctness%: $(SOURCES_CORRECTNESS) $(HEADERS) test/correctness/arith_correctness.c
	$(CC) $(INCLUDES) -DRRLWR_SECURITY_LEVEL=$* $(PACKING_FLAGS) $(CFLAGS) $(TEST_FLAGS) $(SOURCES_CORRECTNESS) test/correctness/arith_correctness.c -o $@

$(SPEED_TARGET): test/test_speed_KEM%: $(SOURCES) $(HEADERS) test/cpucycles.h test/cpucycles.c test/speed_print.c test/speed_print.h $(UTILS_FOLDER)/drng.c $(UTILS_FOLDER)/drng.h test/test_speed_KEM.c
	$(CC) $(INCLUDES) -DRRLWR_SECURITY_LEVEL=$* $(PACKING_FLAGS) $(CFLAGS) $(SOURCES) $(UTILS_FOLDER)/drng.c test/cpucycles.c test/speed_print.c test/test_speed_KEM.c -o $@

$(KAT_TARGET): KAT/KAT_KEM%: $(SOURCES) $(HEADERS) $(UTILS_FOLDER)/drng.c $(UTILS_FOLDER)/drng.h KAT/KAT_KEM.c
	$(CC) $(INCLUDES) -DRRLWR_SECURITY_LEVEL=$* -DOUTPUT_BLANK_TEST_VECTORS=0 -DALGORITHM_INSTANCE=\"KEM_RRLWR$*\" $(PACKING_FLAGS) $(CFLAGS) $(SOURCES) $(UTILS_FOLDER)/drng.c KAT/KAT_KEM.c -o $@

clean:
	-$(RM) -f $(TARGETS)
	-$(RM) -Rf output
