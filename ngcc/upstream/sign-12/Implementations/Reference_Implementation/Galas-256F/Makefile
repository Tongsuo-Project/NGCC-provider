# Makefile for the GALAS reference implementation.
#
# Builds the GF(2^n) self-test first; the full signature library targets are
# added in Phase 3b/3c.

CC      ?= gcc
CFLAGS  ?= -std=c99 -Wall -Wextra -Wpedantic -O2 -D_POSIX_C_SOURCE=200809L
REPS    ?= 100
GALAS_DIR := galas
NGCC_DIR := ngcc

# ---- GF(2^n) self-test ----
test_gf2n: $(GALAS_DIR)/test_gf2n.c $(GALAS_DIR)/gf2n.c $(GALAS_DIR)/gf2n.h
	$(CC) $(CFLAGS) -I$(GALAS_DIR) $< $(GALAS_DIR)/gf2n.c -o $@

# ---- OWF self-test (needs galas_params_<n>.h + owf_vectors.h) ----
test_owf: $(GALAS_DIR)/test_owf.c $(GALAS_DIR)/owf.c $(GALAS_DIR)/owf.h $(GALAS_DIR)/gf2n.c $(GALAS_DIR)/galas_params_*.h $(GALAS_DIR)/owf_vectors.h
	$(CC) $(CFLAGS) -I$(GALAS_DIR) $(GALAS_DIR)/test_owf.c $(GALAS_DIR)/owf.c $(GALAS_DIR)/gf2n.c -o $@

check: test_gf2n test_owf test_xof test_bavc test_vole test_bf test_uh test_qs test_qsgadgets test_ro test_sig_keygen test_sig_roundtrip test_qs_full
	./test_gf2n
	./test_owf
	./test_xof
	./test_bavc
	./test_vole
	./test_bf
	./test_uh
	./test_qs
	./test_qsgadgets
	./test_ro
	./test_sig_keygen
	./test_sig_roundtrip
	./test_qs_full

# ---- bf big-field layer test ----
test_bf: $(GALAS_DIR)/test_bf.c $(GALAS_DIR)/bf.c $(GALAS_DIR)/bf.h $(GALAS_DIR)/gf2n.c $(GALAS_DIR)/bf_vectors.h
	$(CC) $(CFLAGS) -I$(GALAS_DIR) $(GALAS_DIR)/test_bf.c $(GALAS_DIR)/bf.c $(GALAS_DIR)/gf2n.c -o $@

# ---- universal_hashing smoke test ----
test_uh: $(GALAS_DIR)/test_uh.c $(GALAS_DIR)/universal_hashing.c $(GALAS_DIR)/bf.c $(GALAS_DIR)/gf2n.c
	$(CC) $(CFLAGS) -I$(GALAS_DIR) $(GALAS_DIR)/test_uh.c $(GALAS_DIR)/universal_hashing.c $(GALAS_DIR)/bf.c $(GALAS_DIR)/gf2n.c -o $@

# ---- quicksilver authenticated-element arithmetic test ----
test_qs: $(GALAS_DIR)/test_qs.c $(GALAS_DIR)/quicksilver.c $(GALAS_DIR)/bf.c $(GALAS_DIR)/gf2n.c
	$(CC) $(CFLAGS) -I$(GALAS_DIR) $(GALAS_DIR)/test_qs.c $(GALAS_DIR)/quicksilver.c $(GALAS_DIR)/bf.c $(GALAS_DIR)/gf2n.c -o $@

# ---- Gala OWF constraint gadgets test ----
test_qsgadgets: $(GALAS_DIR)/test_qsgadgets.c $(GALAS_DIR)/qs_gadgets.c $(GALAS_DIR)/quicksilver.c $(GALAS_DIR)/owf.c $(GALAS_DIR)/bf.c $(GALAS_DIR)/gf2n.c $(GALAS_DIR)/galas_params_*.h
	$(CC) $(CFLAGS) -I$(GALAS_DIR) $(GALAS_DIR)/test_qsgadgets.c $(GALAS_DIR)/qs_gadgets.c $(GALAS_DIR)/quicksilver.c $(GALAS_DIR)/owf.c $(GALAS_DIR)/bf.c $(GALAS_DIR)/gf2n.c -o $@

# ---- full degree-3 constraint system test ----
test_qs_full: $(GALAS_DIR)/test_qs_full.c $(GALAS_DIR)/qs_full_constraints.c $(GALAS_DIR)/owf.c $(GALAS_DIR)/bf.c $(GALAS_DIR)/gf2n.c $(GALAS_DIR)/galas_params_*.h
	$(CC) $(CFLAGS) -I$(GALAS_DIR) $(GALAS_DIR)/test_qs_full.c $(GALAS_DIR)/qs_full_constraints.c $(GALAS_DIR)/owf.c $(GALAS_DIR)/bf.c $(GALAS_DIR)/gf2n.c -o $@

# ---- Fiat-Shamir random oracle test ----
test_ro: $(GALAS_DIR)/test_ro.c $(GALAS_DIR)/random_oracle.c $(GALAS_DIR)/xof.c $(NGCC_DIR)/auxfunc.c
	$(CC) $(CFLAGS) -I$(GALAS_DIR) -I$(NGCC_DIR) $(GALAS_DIR)/test_ro.c $(GALAS_DIR)/random_oracle.c $(GALAS_DIR)/xof.c $(NGCC_DIR)/auxfunc.c -o $@

# ---- SIG KeyGen end-to-end test (assembles the full SIG interface for keygen) ----
SIG_SRCS = $(GALAS_DIR)/gf2n.c $(GALAS_DIR)/owf.c $(GALAS_DIR)/bf.c $(GALAS_DIR)/xof.c \
           $(GALAS_DIR)/bavc.c $(GALAS_DIR)/vole.c $(GALAS_DIR)/instances.c \
           $(GALAS_DIR)/universal_hashing.c $(GALAS_DIR)/vole_check_ref.c \
           $(GALAS_DIR)/qs_proof_ref.c \
           $(GALAS_DIR)/quicksilver.c $(GALAS_DIR)/qs_gadgets.c \
           $(GALAS_DIR)/random_oracle.c $(GALAS_DIR)/sign_helpers.c $(GALAS_DIR)/challenge.c \
           $(GALAS_DIR)/qs_constraint.c $(GALAS_DIR)/qs_full_constraints.c \
           $(NGCC_DIR)/auxfunc.c $(NGCC_DIR)/drng.c \
           $(NGCC_DIR)/SIG_AlgorithmInstance.c
test_sig_keygen: $(GALAS_DIR)/test_sig_keygen.c $(SIG_SRCS)
	$(CC) $(CFLAGS) -DGALAS_INSTANCE=GALAS_256S -I$(GALAS_DIR) -I$(NGCC_DIR) $(GALAS_DIR)/test_sig_keygen.c $(SIG_SRCS) -o $@

# ---- Sign/Verify round-trip test ----
test_sig_roundtrip: $(GALAS_DIR)/test_sig_roundtrip.c $(SIG_SRCS)
	$(CC) $(CFLAGS) -DGALAS_INSTANCE=GALAS_256S -I$(GALAS_DIR) -I$(NGCC_DIR) $(GALAS_DIR)/test_sig_roundtrip.c $(SIG_SRCS) -o $@

# ---- KAT generator + all instances ----
ALL_INSTANCES = GALAS_160S GALAS_160F GALAS_256S GALAS_256F GALAS_384S GALAS_384F GALAS_512S GALAS_512F
INSTANCE_NAMES = galas-160s galas-160f galas-256s galas-256f galas-384s galas-384f galas-512s galas-512f
PUBLIC_NAMES = Galas-160S Galas-160F Galas-256S Galas-256F Galas-384S Galas-384F Galas-512S Galas-512F

kat_all:
	@i=0; for inst in $(ALL_INSTANCES); do \
		name=$$(echo $(INSTANCE_NAMES) | cut -d' ' -f$$((i+1))); \
		pub=$$(echo $(PUBLIC_NAMES) | cut -d' ' -f$$((i+1))); \
		echo "=== KAT $$name ($$inst) ==="; \
		$(CC) $(CFLAGS) -DGALAS_INSTANCE=$$inst -DGALAS_INSTANCE_NAME=\"$$pub\" -I$(GALAS_DIR) -I$(NGCC_DIR) \
			$(NGCC_DIR)/kat_gen.c $(SIG_SRCS) -o /tmp/kat_$$name 2>/dev/null; \
		/tmp/kat_$$name < ../../API_PKC/Test_Vector/KAT_SIG_AlgorithmInstance.txt > ../../kat/$$name.txt 2>/dev/null; \
		$(CC) $(CFLAGS) -DGALAS_INSTANCE=$$inst -DGALAS_INSTANCE_NAME=\"$$pub\" -I$(GALAS_DIR) -I$(NGCC_DIR) \
			$(NGCC_DIR)/verify_kat.c $(SIG_SRCS) -o /tmp/vkat_$$name 2>/dev/null; \
		/tmp/vkat_$$name < ../../kat/$$name.txt 2>&1 | grep -aE "KAT verify"; \
		i=$$((i+1)); \
	done

kat_gen: $(NGCC_DIR)/kat_gen.c $(SIG_SRCS)
	$(CC) $(CFLAGS) -DGALAS_INSTANCE=GALAS_256S -I$(GALAS_DIR) -I$(NGCC_DIR) $(NGCC_DIR)/kat_gen.c $(SIG_SRCS) -o $@

# ---- All 8 instances roundtrip test ----
test_all: $(GALAS_DIR)/test_all_instances.c $(SIG_SRCS)
	@for inst in $(ALL_INSTANCES); do \
		i=$$(echo "$(ALL_INSTANCES)" | tr ' ' '\n' | grep -n "^$$inst$$" | cut -d: -f1); \
		pub=$$(echo $(PUBLIC_NAMES) | cut -d' ' -f$$i); \
		echo "=== $$inst ==="; \
		$(CC) $(CFLAGS) -DGALAS_INSTANCE=$$inst -DGALAS_INSTANCE_NAME=\"$$pub\" -I$(GALAS_DIR) -I$(NGCC_DIR) \
			$(GALAS_DIR)/test_all_instances.c $(SIG_SRCS) -o /tmp/test_$$inst 2>/dev/null; \
		/tmp/test_$$inst 2>&1 | grep -aE "instance:|keygen|sign|verify|INSTANCE"; \
		echo; \
	done

# ---- Performance benchmark for all instances ----
bench: $(NGCC_DIR)/bench.c $(SIG_SRCS)
	$(CC) $(CFLAGS) -I$(GALAS_DIR) -I$(NGCC_DIR) $(NGCC_DIR)/bench.c $(SIG_SRCS) -o $@
	./bench $(REPS)

bench_all:
	@i=0; for inst in $(ALL_INSTANCES); do \
		name=$$(echo $(INSTANCE_NAMES) | cut -d' ' -f$$((i+1))); \
		pub=$$(echo $(PUBLIC_NAMES) | cut -d' ' -f$$((i+1))); \
		echo "=== $$name ==="; \
		$(CC) $(CFLAGS) -DGALAS_INSTANCE=$$inst -DGALAS_INSTANCE_NAME=\"$$pub\" -I$(GALAS_DIR) -I$(NGCC_DIR) \
			$(NGCC_DIR)/bench.c $(SIG_SRCS) -o /tmp/bench_$$name 2>/dev/null; \
		/tmp/bench_$$name $(REPS) 2>&1; \
		echo; \
		i=$$((i+1)); \
	done

kat: kat_gen verify_kat
	./kat_gen < ../../API_PKC/Test_Vector/KAT_SIG_AlgorithmInstance.txt > /tmp/kat_out.txt 2>/dev/null
	./verify_kat < /tmp/kat_out.txt
	cp /tmp/kat_out.txt ../../kat/galas-256s.txt
	@echo "KAT written to ../../kat/galas-256s.txt"

verify_kat: $(NGCC_DIR)/verify_kat.c $(SIG_SRCS)
	$(CC) $(CFLAGS) -DGALAS_INSTANCE=GALAS_256S -I$(GALAS_DIR) -I$(NGCC_DIR) $(NGCC_DIR)/verify_kat.c $(SIG_SRCS) -o $@

# ---- VOLE commit/reconstruct round-trip ----
test_vole: $(GALAS_DIR)/test_vole.c $(GALAS_DIR)/vole.c $(GALAS_DIR)/bavc.c $(GALAS_DIR)/xof.c $(GALAS_DIR)/instances.c $(NGCC_DIR)/auxfunc.c
	$(CC) $(CFLAGS) -I$(GALAS_DIR) -I$(NGCC_DIR) $(GALAS_DIR)/test_vole.c $(GALAS_DIR)/vole.c $(GALAS_DIR)/bavc.c $(GALAS_DIR)/xof.c $(GALAS_DIR)/instances.c $(NGCC_DIR)/auxfunc.c -o $@

# ---- BAVC round-trip test ----
test_bavc: $(GALAS_DIR)/test_bavc.c $(GALAS_DIR)/bavc.c $(GALAS_DIR)/bavc.h $(GALAS_DIR)/xof.c $(GALAS_DIR)/instances.c $(NGCC_DIR)/auxfunc.c
	$(CC) $(CFLAGS) -I$(GALAS_DIR) -I$(NGCC_DIR) $(GALAS_DIR)/test_bavc.c $(GALAS_DIR)/bavc.c $(GALAS_DIR)/xof.c $(GALAS_DIR)/instances.c $(NGCC_DIR)/auxfunc.c -o $@

# ---- XOF layer smoke test (over NGCC auxfunc pseudoXOF) ----
test_xof: $(GALAS_DIR)/test_xof.c $(GALAS_DIR)/xof.c $(GALAS_DIR)/xof.h $(NGCC_DIR)/auxfunc.c $(NGCC_DIR)/auxfunc.h
	$(CC) $(CFLAGS) -I$(GALAS_DIR) -I$(NGCC_DIR) $(GALAS_DIR)/test_xof.c $(GALAS_DIR)/xof.c $(NGCC_DIR)/auxfunc.c -o $@

clean:
	rm -f test_gf2n bench *.o $(GALAS_DIR)/*.o

.PHONY: check clean bench_all
