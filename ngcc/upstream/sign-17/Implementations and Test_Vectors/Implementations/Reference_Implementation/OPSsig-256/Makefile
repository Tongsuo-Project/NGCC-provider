CC ?= /usr/bin/cc

CFLAGS ?= -O2
CFLAGS += -std=c99 -Wall -Wextra

LDFLAGS ?=
LDLIBS ?=
INSTANCE_NAME ?= $(notdir $(CURDIR))
RESULT_VARIANT ?= reference
SUBMISSION_NAME ?= 数字签名-$(INSTANCE_NAME)-x86-参考实现版
PACKAGE_OUTPUT_DIR ?= $(CURDIR)/../../../Submission_Packages
SELF_EVAL_SCRIPT := ../../../Self_Evaluation/scripts/run_self_eval.sh
PACKAGE_SCRIPT := ../../../Self_Evaluation/scripts/package_submission.sh

SOURCES = \
  sign.c \
  mult.c \
  packing.c \
  polyvec.c \
  poly.c \
  ntt.c \
  reduce.c \
  rounding.c \
  auxfunc.c \
  drng.c

HEADERS = \
  params.h \
  sign.h \
  mult.h \
  packing.h \
  polyvec.h \
  poly.h \
  ntt.h \
  reduce.h \
  rounding.h \
  auxfunc.h \
  drng.h

.PHONY: test speed mem kat selfeval package vectors_test_260602 clean

all: test_OPSsig test_speed_OPSsig

test: test_OPSsig

speed: test_speed_OPSsig

mem: test_mem_OPSsig test_static_mem_OPSsig

kat: kat_sig
	./kat_sig

selfeval:
	$(SELF_EVAL_SCRIPT) "$(CURDIR)" "$(RESULT_VARIANT)" "$(INSTANCE_NAME)" "$(SUBMISSION_NAME)"

package:
	$(PACKAGE_SCRIPT) "$(CURDIR)" "$(SUBMISSION_NAME)" "$(RESULT_VARIANT)" "$(PACKAGE_OUTPUT_DIR)"

test_OPSsig: test_OPSsig.c $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $< $(SOURCES) $(LDFLAGS) $(LDLIBS)

test_speed_OPSsig: test_speed_OPSsig.c $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $< $(SOURCES) $(LDFLAGS) -lrt $(LDLIBS)

test_mem_OPSsig: test_mem_OPSsig.c $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $< $(SOURCES) $(LDFLAGS) $(LDLIBS)

test_static_mem_OPSsig: test_static_mem_OPSsig.c $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $< $(SOURCES) $(LDFLAGS) $(LDLIBS)

kat_sig: KAT_SIG.c SIG_AlgorithmInstance.c $(SOURCES) $(HEADERS) SIG_AlgorithmInstance.h
	$(CC) $(CFLAGS) -o $@ KAT_SIG.c SIG_AlgorithmInstance.c $(SOURCES) $(LDFLAGS) $(LDLIBS)

vectors_test_260602: test_vectors-260602
	./test_vectors-260602

test_vectors-260602: test_vectors.c $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $< $(SOURCES) $(LDFLAGS) $(LDLIBS)

clean:
	rm -f test_OPSsig test_speed_OPSsig test_mem_OPSsig test_static_mem_OPSsig kat_sig test_vectors-260602
