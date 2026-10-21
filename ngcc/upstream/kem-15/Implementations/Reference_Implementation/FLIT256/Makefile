CC      ?= gcc
CFLAGS  ?= -O3 -march=native -fomit-frame-pointer -std=c11 -Wall -Wextra
DEPFLAGS = -MMD -MP

# ── Source files ──────────────────────────────────────────────
CORE      = ntt.c poly.c decode.c packing.c reduce.c verify.c auxfunc.c drng.c
INDCPA    = indcpa.c
KAT_SRCS  = KAT_KEM.c KEM_AlgorithmInstance.c
ALL_SRCS  = kem.c $(CORE) $(INDCPA) $(KAT_SRCS)
HEADERS   = KEM_AlgorithmInstance.h config.h decode.h indcpa.h kem.h ntt.h params.h \
            poly.h packing.h reduce.h verify.h symmetric.h auxfunc.h drng.h

# ── Test targets ──────────────────────────────────────────────
TESTDIR   = test
TEST_MUL  = $(TESTDIR)/test_mul
TEST_CPAP = $(TESTDIR)/test_cpapke
TEST_KEM  = $(TESTDIR)/test_kem
TEST_SPD  = $(TESTDIR)/test_speed

# ── Build rules ───────────────────────────────────────────────
.PHONY: all test speed clean help

all: KAT_KEM

KAT_KEM: $(HEADERS) $(ALL_SRCS)
	$(CC) $(CFLAGS) $(DEPFLAGS) -o $@ $(ALL_SRCS)

# Tests that don't need INDCPA
$(TEST_MUL): $(TESTDIR)/test_mul.c $(CORE)
	$(CC) $(CFLAGS) $(DEPFLAGS) -o $@ $^

# Tests that need INDCPA
$(TEST_CPAP): $(TESTDIR)/test_cpapke.c $(CORE) $(INDCPA)
	$(CC) $(CFLAGS) $(DEPFLAGS) -o $@ $^

$(TEST_KEM): $(TESTDIR)/test_kem.c kem.c $(CORE) $(INDCPA)
	$(CC) $(CFLAGS) $(DEPFLAGS) -o $@ $^

$(TEST_SPD): $(TESTDIR)/test_speed.c $(TESTDIR)/cpucycles.c $(TESTDIR)/speed_print.c \
             kem.c $(CORE) $(INDCPA)
	$(CC) $(CFLAGS) $(DEPFLAGS) -o $@ $^

test: $(TEST_MUL) $(TEST_CPAP) $(TEST_KEM) $(TEST_SPD)

speed: $(TEST_SPD)
	@echo "=== FLIT Reference speed benchmark ==="
	./$(TEST_SPD)

clean:
	-rm -f KAT_KEM $(TEST_MUL) $(TEST_CPAP) $(TEST_KEM) $(TEST_SPD)
	-rm -f *.o *.d $(TESTDIR)/*.d
	-rm -rf output

help:
	@echo "FLIT Reference Implementation"
	@echo ""
	@echo "Targets:"
	@echo "  all      Build KAT_KEM binary (default)"
	@echo "  test     Build all test binaries"
	@echo "  speed    Build and run speed benchmark"
	@echo "  clean    Remove all build artifacts"
	@echo "  help     Show this help"
	@echo ""
	@echo "Variables:"
	@echo "  CC       C compiler (default: gcc)"
	@echo "  CFLAGS   Compiler flags"

# ── Auto dependency tracking ──────────────────────────────────
DEPFILES = $(ALL_SRCS:.c=.d) \
           $(TESTDIR)/test_mul.d \
           $(TESTDIR)/test_cpapke.d \
           $(TESTDIR)/test_kem.d \
           $(TESTDIR)/test_speed.d
-include $(DEPFILES)
