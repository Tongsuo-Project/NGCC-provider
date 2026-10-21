CC = gcc
CFLAGS += -O3
CFLAGS += -Wall -Wextra -Wpedantic -Wmissing-prototypes -Wredundant-decls \
  -Wshadow -Wpointer-arith -fomit-frame-pointer
RM = /bin/rm

BASE_FOLDER = .
ARITH_FOLDER = ./arith
UTILS_FOLDER = ./utils
INCLUDES = -I$(BASE_FOLDER) -I$(UTILS_FOLDER) -I$(ARITH_FOLDER)

override SECURITY_LEVEL = 256

SOURCES_RRLWR = $(ARITH_FOLDER)/fprime.c $(ARITH_FOLDER)/poly.c $(ARITH_FOLDER)/ring.c $(ARITH_FOLDER)/packing.c $(ARITH_FOLDER)/uniform.c sign_ring.c sign.c crt.c
SOURCES_UTILS = $(UTILS_FOLDER)/auxfunc.c $(UTILS_FOLDER)/drng.c
SOURCES = $(SOURCES_RRLWR) $(SOURCES_UTILS)
BREAKDOWN_SOURCES = $(filter-out sign.c,$(SOURCES_RRLWR)) sign_test.c $(SOURCES_UTILS)

HEADERS_RRLWR = $(ARITH_FOLDER)/fprime.h $(ARITH_FOLDER)/poly.h $(ARITH_FOLDER)/ring.h $(ARITH_FOLDER)/packing.h $(ARITH_FOLDER)/uniform.h parameters.h hash_domain.h sign_ring.h sign.h crt.h
HEADERS_UTILS = $(UTILS_FOLDER)/auxfunc.h $(UTILS_FOLDER)/drng.h
HEADERS = $(HEADERS_RRLWR) $(HEADERS_UTILS)

TEST_TARGETS = test/test_SIGN$(SECURITY_LEVEL)
UNIT_TEST_TARGETS = test/unit_tests_SIGN$(SECURITY_LEVEL)
SPEED_TARGETS = test/test_speed_SIGN$(SECURITY_LEVEL)
KAT_TARGETS = KAT/KAT_SIGN$(SECURITY_LEVEL)
BREAKDOWN_TARGETS = test/test_breakdown_SIGN$(SECURITY_LEVEL)
TARGETS = $(TEST_TARGETS) $(UNIT_TEST_TARGETS) $(SPEED_TARGETS) $(KAT_TARGETS) $(BREAKDOWN_TARGETS)

.PHONY: all test speed KAT breakdown clean

all: test speed KAT breakdown

test: $(TEST_TARGETS) $(UNIT_TEST_TARGETS)
	@if [ "$(run)" = "1" ]; then \
		./$(TEST_TARGETS) && \
		./$(UNIT_TEST_TARGETS); \
	fi

speed: $(SPEED_TARGETS)
	@if [ "$(run)" = "1" ]; then \
		./$(SPEED_TARGETS); \
	fi

KAT: $(KAT_TARGETS)
	@if [ "$(run)" = "1" ]; then \
		./$(KAT_TARGETS); \
	fi

breakdown: $(BREAKDOWN_TARGETS)
	@if [ "$(run)" = "1" ]; then \
		./$(BREAKDOWN_TARGETS); \
	fi

$(TEST_TARGETS): test/test_SIGN%: $(SOURCES) $(HEADERS) test/test_SIGN.c
	$(CC) $(INCLUDES) -DRRLWR_SECURITY_LEVEL=$* -DMEASURE_HEURISTICS $(CFLAGS) $(SOURCES) test/test_SIGN.c -o $@

$(UNIT_TEST_TARGETS): test/unit_tests_SIGN%: $(SOURCES) $(HEADERS) test/unit_tests_SIGN.c
	$(CC) $(INCLUDES) -DRRLWR_SECURITY_LEVEL=$* $(CFLAGS) $(SOURCES) test/unit_tests_SIGN.c -o $@

$(SPEED_TARGETS): test/test_speed_SIGN%: $(SOURCES) $(HEADERS) test/cpucycles.h test/cpucycles.c test/test_speed_SIGN.c
	$(CC) $(INCLUDES) -DRRLWR_SECURITY_LEVEL=$* $(CFLAGS) $(SOURCES) test/cpucycles.c test/test_speed_SIGN.c -o $@

$(KAT_TARGETS): KAT/KAT_SIGN%: $(SOURCES) $(HEADERS) KAT/KAT_SIG.c
	$(CC) $(INCLUDES) -DRRLWR_SECURITY_LEVEL=$* $(CFLAGS) $(SOURCES) KAT/KAT_SIG.c -o $@

$(BREAKDOWN_TARGETS): test/test_breakdown_SIGN%: $(BREAKDOWN_SOURCES) $(HEADERS) sign_test.h test/cpucycles.h test/cpucycles.c test/test_breakdown_SIGN.c
	$(CC) $(INCLUDES) -DRRLWR_SECURITY_LEVEL=$* -DRRLWR_SIGN_BREAKDOWN $(CFLAGS) $(BREAKDOWN_SOURCES) test/cpucycles.c test/test_breakdown_SIGN.c -o $@

clean:
	-$(RM) -f $(TARGETS)
	-$(RM) -Rf output
