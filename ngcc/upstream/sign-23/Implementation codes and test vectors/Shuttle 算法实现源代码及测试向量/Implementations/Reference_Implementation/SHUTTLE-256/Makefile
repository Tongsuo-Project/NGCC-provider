# Makefile for the SHUTTLE signature REFERENCE implementation (pure C99 scalar).
#
#   make                 # build all test binaries for all modes -> out/
#   make check           # build + run every test for every mode (assert exit 0)
#   make tables          # regenerate all @@AUTOGEN@@ regions from ../tools
#   make check-consts    # reproducibility/drift gate (re-run generators, cmp)
#   make check-warn      # ref pedantic+Werror warning gate
#   make speed speed_txt profile profile_txt   # benchmarks (sources land later)
#   make ct-scan         # constant-time scan
#   make clean
#
# MODES selects the parameter sets (default: all three).  Each binary is one
# parameter set (-DSHUTTLE_MODE=$m); there is no runtime mode dispatch.
#
# XOF MODE axis: a `MODE` make-var of `SHA3` injects -DSHA3_MODE (SHAKE
# backend); the default is the NGCC SM3 Hash-DRBG backend (no extra -D).
#
# Generated constants live in @@AUTOGEN@@ regions inside committed headers
# (params.h params/bounds), so a plain build needs no Python; `make tables`
# regenerates and `make check-consts` proves the committed bytes do not drift.

MODES ?= 128 256 512
# Test suite: t_params + test_xof (XOF build-verify), t_reduce + test_ntt
# (reduce/NTT), test_pack (poly arith + all (de)serialization), and the rest.
TESTS ?= t_params test_xof t_reduce test_ntt test_pack test_sampler test_approx test_samplers \
         test_irs test_rounding test_rans test_sign test_kat
CC    ?= gcc

# Exact NGCC Reference flags: pure ISO C99, NO asm/intrinsics,
# MUST be warning-clean.  -Werror is added by the `check-warn` gate.
CFLAGS ?= -std=c99 -Wpedantic -Wall -Wextra -O2 -I. -Itest -I../tools
LDLIBS ?=
BINDIR  = out
CT_OPT ?= -O2

# XOF MODE switch (NGCC_MODE default vs SHA3_MODE).  `make MODE=SHA3 ...`.
MODE ?=
ifeq ($(MODE),SHA3)
MODEFLAG = -DSHA3_MODE
else
MODEFLAG =
endif

# Per-test source lists (POSIX-sh strings).  t_params is header-only.  Each
# entry maps a test to its source list, e.g. 'test_ntt=reduce.c ntt_ref.c'.
#
# test_xof: the unified-XOF build-verify.  Its source list is MODE-
# dependent -- SHA3_MODE links the vendored Keccak (symmetric.c + fips202.c),
# the default NGCC_MODE links the SM3 DRBG (symmetric.c + drng.c + auxfunc.c).
# The MODE-selected list is injected below ($(XOF_SRCS)).
ifeq ($(MODE),SHA3)
XOF_SRCS = symmetric.c fips202.c
# The NGCC KAT harness (SIG_AlgorithmInstance.c / KAT_SIG.c / test_kat) ALWAYS
# seeds via the SM3 Hash-DRBG drng_algorithm (the fixed competition protocol),
# independent of the scheme's xof MODE.  So a KAT binary under SHA3_MODE needs
# drng.c+auxfunc.c (seeding) IN ADDITION to the SHAKE xof backend.
KAT_XOF = symmetric.c fips202.c drng.c auxfunc.c
else
XOF_SRCS = symmetric.c drng.c auxfunc.c
KAT_XOF = $(XOF_SRCS)
endif

SH_SRCS = \
't_params=' \
"test_xof=$(XOF_SRCS)" \
't_reduce=reduce.c' \
'test_ntt=reduce.c poly_ntt.c' \
'test_pack=packing.c poly.c reduce.c rans.c' \
'test_sampler=sampler.c reduce.c' \
'test_approx=approx_exp.c approx_log.c' \
"test_samplers=polyvec.c sampler.c reduce.c approx_exp.c approx_log.c $(XOF_SRCS)" \
"test_irs=irs.c sampler_u.c approx_exp.c approx_log.c reduce.c $(XOF_SRCS)" \
'test_rounding=rounding.c reduce.c poly.c poly_ntt.c packing.c rans.c' \
'test_rans=packing.c poly.c reduce.c rans.c' \
"test_sign=sign.c polyvec.c sampler.c sampler_u.c irs.c rounding.c packing.c poly.c poly_ntt.c reduce.c rans.c approx_exp.c approx_log.c $(XOF_SRCS)" \
"test_kat=SIG_AlgorithmInstance.c sign.c polyvec.c sampler.c sampler_u.c irs.c rounding.c packing.c poly.c poly_ntt.c reduce.c rans.c approx_exp.c approx_log.c $(KAT_XOF)"

# Per-set NTT vendoring wiring: the per-mode scalar NTT lives under
# ntt/<qset>/.  A test whose source list contains poly_ntt.c (the scalar shim)
# or polyvec.c gets the matching scalar oracle ntt/<qset>/ntt_ref.c
# appended, and -Intt/<qset> added so poly_ntt.h finds the per-set header.
QSET_128 = q15361n256
QSET_256 = q61441n512
QSET_512 = q59393n1024

.PHONY: all check tables check-consts check-warn check-ct-reduce speed speed_txt profile profile_txt ct-scan clean

# Negative gate: the constant-time reduce substrate runs on SECRET NTT
# residues, so reduce.c MUST contain no division mnemonic (the Barrett path is
# the whole point) -- assert via objdump.  The static-inline uint16 core lowers
# to CMOV (branchless) on -O2; this gate covers the out-of-line Barrett helpers.
check-ct-reduce:
	@mkdir -p $(BINDIR)
	@for m in $(MODES); do \
	  $(CC) $(CFLAGS) -DSHUTTLE_MODE=$$m -c reduce.c -o $(BINDIR)/reduce_$$m.o || exit 1; \
	  if objdump -d $(BINDIR)/reduce_$$m.o | grep -iqE '\b(idiv|div)[lq]?\b'; then \
	    echo "  FAIL: division mnemonic in reduce.c (mode $$m)"; exit 1; \
	  else echo "  PASS: reduce.c mode=$$m division-free (constant-time Barrett)"; fi; \
	done
	@echo "check-ct-reduce: reduce.c is division-free for all modes"

all:
	@mkdir -p $(BINDIR)
	@for m in $(MODES); do \
	  case $$m in 128) qs=$(QSET_128);; 256) qs=$(QSET_256);; 512) qs=$(QSET_512);; esac; \
	  for entry in $(SH_SRCS); do \
	    t=$${entry%%=*}; srcs=$${entry#*=}; \
	    for want in $(TESTS); do [ "$$want" = "$$t" ] || continue; \
	      extra=""; inc=""; \
	      case "$$srcs" in \
	        *poly_ntt.c*|*polyvec.c*) extra="ntt/$$qs/ntt_ref.c"; inc="-Intt/$$qs";; \
	      esac; \
	      stats=""; \
	      case "$$t" in trace_kat) stats="-DTEST_RANS_STATS";; diff_fuzz) stats="-DDIFF_FUZZ_N=$(DIFF_FUZZ_N)";; test_sign) stats="-DSIG_RAW";; esac; \
	      $(CC) $(CFLAGS) $$inc $(MODEFLAG) $$stats -DSHUTTLE_MODE=$$m test/$$t.c $$srcs $$extra $(LDLIBS) \
	        -o $(BINDIR)/$${t}_$${m} || exit 1; \
	    done; \
	  done; \
	done
	@echo "built: [$(TESTS)] x [$(MODES)]"

check: all
	@for m in $(MODES); do \
	  echo "=== SHUTTLE-$$m (ref) ==="; \
	  for want in $(TESTS); do \
	    [ -x $(BINDIR)/$${want}_$${m} ] \
	      || { echo "missing test binary: $(BINDIR)/$${want}_$${m}"; exit 1; }; \
	    $(BINDIR)/$${want}_$${m} || exit 1; \
	  done; \
	done

# Regenerate EVERY @@AUTOGEN@@ region.  gen_params.py + gen_bounds.py seed the
# params.h regions; the other generators follow (NTT consts are regenerated by
# the per-qset tools/ntt/<qset>/gen_ntt.py -- there is no gen_zetas.py).
tables:
	cd ../tools && python3 gen_params.py && python3 gen_bounds.py \
	  && python3 gen_rcdt.py && python3 gen_approx_exp_poly.py && python3 gen_approx_log_poly.py \
	  && python3 gen_irs_consts.py && python3 gen_rounding.py \
	  && python3 gen_rans_tables.py && python3 SigSize.py
	@echo "tables: regenerated params + bounds + rcdt + approx + irs + rounding + rans"

# Reproducible-constants drift gate (SEPARATE from `tables`).  Back up each
# generated header, re-run its generator, cmp against the committed bytes, FAIL
# on drift.  Covers params.h (params + bounds) and every other generated region.
check-consts:
	@mkdir -p $(BINDIR)
	@echo "== params.h (@@AUTOGEN:params@@ + @@AUTOGEN:bounds@@; gen_params.py + gen_bounds.py) =="
	@cp params.h $(BINDIR)/params.h.bak
	@( cd ../tools && python3 gen_params.py >/dev/null 2>&1 && python3 gen_bounds.py >/dev/null 2>&1 ) \
	  || { echo "  FAIL: generator error"; rm -f $(BINDIR)/params.h.bak; exit 1; }
	@cmp -s params.h $(BINDIR)/params.h.bak \
	  && echo "  PASS: params.h reproducible (params + bounds drift-free)" \
	  || { echo "  FAIL: params.h drift"; diff $(BINDIR)/params.h.bak params.h; rm -f $(BINDIR)/params.h.bak; exit 1; }
	@rm -f $(BINDIR)/params.h.bak
	@echo "== vendored NTT (gen_ntt.py per <qset>; the AVX2/AVX512 .S + consts) =="
	@rm -rf $(BINDIR)/ntt_regen
	@mkdir -p $(BINDIR)/ntt_regen
	@for qs in $(QSET_128) $(QSET_256) $(QSET_512); do \
	  python3 ../tools/ntt/$$qs/gen_ntt.py $(BINDIR)/ntt_regen >/dev/null 2>&1 \
	    || { echo "  FAIL: gen_ntt.py error ($$qs)"; exit 1; }; \
	  for f in avx2/$$qs/ntt.S avx2/$$qs/ntt_consts.c \
	           avx512/$$qs/ntt_avx512.S avx512/$$qs/ntt_consts_avx512.c; do \
	    cmp -s ../$$f $(BINDIR)/ntt_regen/$$f \
	      || { echo "  FAIL: NTT drift in $$f"; exit 1; }; \
	  done; \
	  echo "  PASS: $$qs NTT .S/consts reproducible (4 files, AVX2+AVX512)"; \
	done
	@rm -rf $(BINDIR)/ntt_regen
	@echo "== rcdt_tables.h (gen_rcdt.py --check) + approx headers (gen_approx_*.py) =="
	@( cd ../tools && python3 gen_rcdt.py --check >/dev/null 2>&1 ) \
	  && echo "  PASS: rcdt_tables.h reproducible" \
	  || { echo "  FAIL: rcdt_tables.h drift"; exit 1; }
	@cp ../tools/approx_exp_poly.h $(BINDIR)/aexp.bak; cp ../tools/approx_log_poly.h $(BINDIR)/alog.bak
	@( cd ../tools && python3 gen_approx_exp_poly.py >/dev/null 2>&1 && python3 gen_approx_log_poly.py >/dev/null 2>&1 ) \
	  || { echo "  FAIL: approx generator error"; exit 1; }
	@cmp -s ../tools/approx_exp_poly.h $(BINDIR)/aexp.bak && cmp -s ../tools/approx_log_poly.h $(BINDIR)/alog.bak \
	  && echo "  PASS: approx_exp_poly.h + approx_log_poly.h reproducible" \
	  || { echo "  FAIL: approx header drift"; exit 1; }
	@rm -f $(BINDIR)/aexp.bak $(BINDIR)/alog.bak
	@echo "== approx precision gates (quadmath verifiers: exp >=53 bit, log >=57 bit) =="
	@$(CC) -O2 -I. -I../tools ../tools/verify_approx_exp_poly.c -lquadmath -lm -o $(BINDIR)/vexp 2>/dev/null \
	  && $(BINDIR)/vexp >/dev/null 2>&1 && echo "  PASS: ApproxExp >= 53-bit + x4==scalar" \
	  || { echo "  FAIL: ApproxExp precision gate"; exit 1; }
	@$(CC) -O2 -I. -I../tools ../tools/verify_approx_log_poly.c -lquadmath -lm -o $(BINDIR)/vlog 2>/dev/null \
	  && $(BINDIR)/vlog >/dev/null 2>&1 && echo "  PASS: ApproxLog >= 57-bit + monotone + pin + CT-scan + x2==scalar" \
	  || { echo "  FAIL: ApproxLog precision gate"; exit 1; }
	@rm -f $(BINDIR)/vexp $(BINDIR)/vlog
	@echo "== irs.c (gen_irs_consts.py) + rounding_consts.h (gen_rounding.py) + rans.h (gen_rans_tables.py+SigSize.py) =="
	@cp irs.c $(BINDIR)/irs.bak; cp ../tools/rounding_consts.h $(BINDIR)/rnd.bak; cp rans.h $(BINDIR)/rans.bak
	@( cd ../tools && python3 gen_irs_consts.py >/dev/null 2>&1 && python3 gen_rounding.py >/dev/null 2>&1 \
	     && python3 gen_rans_tables.py >/dev/null 2>&1 && python3 SigSize.py >/dev/null 2>&1 ) \
	  || { echo "  FAIL: irs/rounding/rans generator error"; exit 1; }
	@cmp -s irs.c $(BINDIR)/irs.bak && echo "  PASS: irs.c (2r^2 ln2 const) reproducible" \
	  || { echo "  FAIL: irs.c drift"; exit 1; }
	@cmp -s ../tools/rounding_consts.h $(BINDIR)/rnd.bak && echo "  PASS: rounding_consts.h reproducible" \
	  || { echo "  FAIL: rounding_consts.h drift"; exit 1; }
	@cmp -s rans.h $(BINDIR)/rans.bak && echo "  PASS: rans.h (rANS tables + reserve) reproducible" \
	  || { echo "  FAIL: rans.h drift"; exit 1; }
	@rm -f $(BINDIR)/irs.bak $(BINDIR)/rnd.bak $(BINDIR)/rans.bak
	@echo "check-consts: all generated constants reproducible"

# Reference warning gate: the ref build MUST be clean under -Werror -Wpedantic
# (the hard NGCC "no warnings" expectation for Reference_Implementation).
check-warn:
	@mkdir -p $(BINDIR)
	@for m in $(MODES); do \
	  case $$m in 128) qs=$(QSET_128);; 256) qs=$(QSET_256);; 512) qs=$(QSET_512);; esac; \
	  for entry in $(SH_SRCS); do \
	    t=$${entry%%=*}; srcs=$${entry#*=}; \
	    for want in $(TESTS); do [ "$$want" = "$$t" ] || continue; \
	      extra=""; inc=""; \
	      case "$$srcs" in \
	        *poly_ntt.c*|*polyvec.c*) extra="ntt/$$qs/ntt_ref.c"; inc="-Intt/$$qs";; \
	      esac; \
	      $(CC) $(CFLAGS) $$inc -Werror $(MODEFLAG) -DSHUTTLE_MODE=$$m test/$$t.c $$srcs $$extra $(LDLIBS) \
	        -o $(BINDIR)/$${t}_$${m}_warn || { echo "  FAIL: $$t mode=$$m has warnings"; exit 1; }; \
	    done; \
	  done; \
	done
	@echo "check-warn: ref builds clean under -Werror -Wpedantic -Wall -Wextra"

# KAT regression lock.  test_kat replays the KAT_SIG.c byte protocol,
# FNV-1a hashes the pk||sk||sn transcript, asserts verify-accept + tamper-reject,
# prints KAT_hash.  These are the production rANS-path hashes (test_kat builds
# WITHOUT -DSIG_RAW).  Re-record on any output-changing diff (samplers/encoding).
# RAW-path hashes (for -DSIG_RAW debugging) are kept in tools/pyref/README.md.
KAT_128_NGCC = 5525731303976652954
KAT_256_NGCC = 4684222326669620474
KAT_512_NGCC = 8059965594724964533
KAT_128_SHA3 = 11439871523140599355
KAT_256_SHA3 = 8372293911548186006
KAT_512_SHA3 = 12332646238986233852
MODELABEL = $(if $(filter SHA3,$(MODE)),SHA3,NGCC)
check-kat: all
	@KAT_128=$(KAT_128_$(MODELABEL)); KAT_256=$(KAT_256_$(MODELABEL)); KAT_512=$(KAT_512_$(MODELABEL)); \
	for m in $(MODES); do \
	  got=`$(BINDIR)/test_kat_$$m | sed -n 's/^KAT_hash=//p'`; \
	  eval exp=\$$KAT_$$m; \
	  if [ "$$got" = "$$exp" ]; then echo "  PASS: KAT-$$m ($(MODELABEL)) = $$got"; \
	  else echo "  FAIL: KAT-$$m ($(MODELABEL)) got $$got want $$exp"; exit 1; fi; \
	done
	@echo "check-kat: recorded KAT hashes reproduce ($(MODELABEL), rANS path)"

# The full scheme source list driving the sig_* interface (SIG_AlgorithmInstance
# is the NGCC adapter; the XOF backend is appended MODE-dependently as $(KAT_XOF)
# -- the speed/profile drivers seed via the SM3 Hash-DRBG exactly like the KAT
# harness, so they need drng.c+auxfunc.c even under SHA3_MODE).
SCHEME_SRC = SIG_AlgorithmInstance.c sign.c polyvec.c sampler.c sampler_u.c irs.c \
             rounding.c packing.c poly.c poly_ntt.c reduce.c rans.c approx_exp.c approx_log.c
SPEED_SRCS   = $(SCHEME_SRC)
PROFILE_SRCS = $(SCHEME_SRC)
# Extra cc flags for the bench drivers ONLY (e.g. -DBENCH_SIGN_RUNS=N -DNV=N
# -DNGCC_ITERS=N to shrink the run for quick/CI evidence); empty by default.
SPEED_EXTRA ?=

# End-to-end keygen/sign/verify cycle benchmark (scalar reference).  Mirrors the
# `all` target's per-mode build (qs select -> -Intt/$$qs + ntt/$$qs/ntt_ref.c).
#
# `speed` runs the full bench under BOTH XOF backends -- NGCC (SM3 Hash-DRBG)
# first, then SHA3 (SHAKE) -- via two recursive sub-makes; speed_run is the
# single-XOF-mode worker (the MODE make-var selects the backend).  speed_txt
# captures both passes into one test/speed.txt.
.PHONY: speed speed_run
speed:
	@$(MAKE) --no-print-directory speed_run MODE=
	@$(MAKE) --no-print-directory speed_run MODE=SHA3
speed_run:
	@mkdir -p $(BINDIR)
	@echo "-- XOF mode: $(MODELABEL) --"
	@for m in $(MODES); do \
	  case $$m in 128) qs=$(QSET_128);; 256) qs=$(QSET_256);; 512) qs=$(QSET_512);; esac; \
	  $(CC) $(CFLAGS) -Intt/$$qs $(MODEFLAG) -DSHUTTLE_MODE=$$m $(SPEED_EXTRA) \
	    test/speed_sign.c test/cpucycles.c $(SPEED_SRCS) $(KAT_XOF) ntt/$$qs/ntt_ref.c \
	    $(LDLIBS) -lm -o $(BINDIR)/speed_sign_$$m || exit 1; \
	  $(BINDIR)/speed_sign_$$m; echo; \
	done

# Human-readable machine-info preamble shared by speed_txt / profile_txt.
MACHINFO = printf ' date    : '; date -u '+%Y-%m-%dT%H:%M:%SZ'; \
           printf ' kernel  : '; uname -sr; \
           printf ' cpu     : '; ( lscpu 2>/dev/null | sed -n 's/^Model name:[[:space:]]*//p' | head -1 || grep -m1 'model name' /proc/cpuinfo | sed 's/.*: //' ); \
           printf ' cc      : '; $(CC) --version | head -1; \
           printf ' cflags  : %s\n' '$(CFLAGS)'

speed_txt:
	@{ echo "=== SHUTTLE  ref/  end-to-end speed (tail/median rdtsc cycles, speed_sign) ==="; \
	   echo " xof modes: NGCC then SHA3 (both passes below)"; \
	   $(MACHINFO); \
	   echo " stability metrics: p25/p50/p75/p90/p99 percentiles; StQ1/StQ2/StQ3 stabilized quartiles;"; \
	   echo "   stddev / cv (stddev/avg); outlier_count(>p50*1.5); p90/p50 & p99/p50 tail ratios."; \
	   echo "   Reference: D. J. Bernstein, Robust statistics for rejection-sampling timings."; \
	   echo; echo "-- pure-scalar reference --"; } > test/speed.txt
	@$(MAKE) --no-print-directory speed >> test/speed.txt
	@echo "wrote test/speed.txt"; cat test/speed.txt

# Per-component time + randomness profile of crypto_sign (-DPROF_TIME
# -DPROF_RAND; the prof.c TU supplies the counters + report).  Same per-mode
# build as `speed`, plus test/prof.c.
# `profile` runs both XOF backends (NGCC then SHA3) like `speed`; profile_run
# is the single-XOF-mode worker.
.PHONY: profile profile_run
profile:
	@$(MAKE) --no-print-directory profile_run MODE=
	@$(MAKE) --no-print-directory profile_run MODE=SHA3
profile_run:
	@mkdir -p $(BINDIR)
	@echo "-- XOF mode: $(MODELABEL) --"
	@for m in $(MODES); do \
	  case $$m in 128) qs=$(QSET_128);; 256) qs=$(QSET_256);; 512) qs=$(QSET_512);; esac; \
	  $(CC) $(CFLAGS) -DPROF_TIME -DPROF_RAND -Intt/$$qs $(MODEFLAG) -DSHUTTLE_MODE=$$m \
	    test/speed_profile.c test/prof.c $(PROFILE_SRCS) $(KAT_XOF) ntt/$$qs/ntt_ref.c \
	    $(LDLIBS) -lm -o $(BINDIR)/speed_profile_$$m || exit 1; \
	  $(BINDIR)/speed_profile_$$m; echo; \
	done

profile_txt:
	@{ echo "=== SHUTTLE  ref/  per-component profile (cycles/op, make profile) ==="; \
	   echo " xof modes: NGCC then SHA3 (both passes below)"; \
	   $(MACHINFO); echo; echo "-- pure-scalar reference --"; } > test/profiling.txt
	@$(MAKE) --no-print-directory profile >> test/profiling.txt
	@echo "wrote test/profiling.txt"

# Constant-time scan: delegate to the shared scanner.
ct-scan:
	@if [ -f ../tools/ct_scan.py ]; then \
	  python3 ../tools/ct_scan.py --backends ref --modes "$(MODES)" --ccs "$(CC)" --opts="$(CT_OPT)" --report test/ct_scan_ref.txt; \
	else \
	  echo "ct-scan: deferred (tools/ct_scan.py not present)"; \
	fi

clean:
	rm -rf $(BINDIR)
