# QingLuan-128 — API_PKC KAT generator + official-KAT validator.
#
# Self-contained within this AlgorithmInstance folder (algorithm-requirements §6):
# the QingLuan core is pulled from ../src + ../include of THIS instance only;
# the API_PKC framework (drng/auxfunc/KAT_SIG, unmodified) and the adapter layer
# live here. For KAT GENERATION, hash.c and utils.c are REPLACED by
# hash_adapter.c / utils_adapter.c, so randomness comes from the framework DRNG.
#
# Level is fixed to 128 (matches ../include/params.h). -DQINGLUAN_128 is required
# only so the framework driver names the output KAT_SIG_QingLuan-128.txt.
#
# Targets:
#   make / make run  -> output/KAT_SIG_QingLuan-128.txt  (generate official KAT)
#   make verify      -> validate the submission-root official vectors through the
#                       REFERENCE verifier (../src, verify-only; no regeneration)
CC = gcc
LEVEL = 128
CFLAGS = -Wall -Wextra -std=c11 -O2 -DQINGLUAN_$(LEVEL)
# -I. makes this folder's hash.h shadow ../include/hash.h (private adapter structs).
INCLUDES = -I. -I../include
LDFLAGS =
ifeq ($(OS),Windows_NT)
    EXE_EXT = .exe
    VERIFY_LIBS = -lbcrypt
endif

# QingLuan core from THIS instance (hash.c/utils.c excluded: KAT generation uses
# the adapter hash/utils).
QL_SRCS = ../src/fq_arith.c ../src/rsdp.c ../src/restr.c ../src/mpc.c \
          ../src/keygen.c ../src/sign.c ../src/verify.c
# API_PKC framework (unmodified) + adapter layer (local to this folder).
FW_SRCS = drng.c auxfunc.c KAT_SIG.c
AD_SRCS = SIG_AlgorithmInstance.c utils_adapter.c hash_adapter.c
SRCS = $(QL_SRCS) $(FW_SRCS) $(AD_SRCS)
TARGET = qingluan_kat_api$(EXE_EXT)

# Official-KAT validator: full REFERENCE build (../src, incl. the reference
# hash.c/utils.c) + kat_verify.c; consumes the published vectors, verify-only.
REF_SRCS = ../src/fq_arith.c ../src/restr.c ../src/rsdp.c ../src/mpc.c \
           ../src/hash.c ../src/utils.c ../src/keygen.c ../src/sign.c \
           ../src/verify.c ../src/selftest.c
VERIFY_TARGET = kat_verify$(EXE_EXT)
OFFICIAL_KAT = ../../../../Test_Vectors/KAT_SIG_QingLuan-$(LEVEL).txt

all: $(TARGET)

# One-shot compile: no intermediate .o files (keeps ../src clean).
$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(SRCS) $(LDFLAGS)

# Generates output/KAT_SIG_QingLuan-128.txt (NIST/API_PKC framework format).
run: $(TARGET)
	./$(TARGET)

# Validate via the reference verifier. NB: -I../include only (NOT -I.) so the
# REFERENCE hash.h/hash.c are linked, not the adapter's.
$(VERIFY_TARGET): kat_verify.c $(REF_SRCS)
	$(CC) $(CFLAGS) -I../include -o $@ kat_verify.c $(REF_SRCS) $(LDFLAGS) $(VERIFY_LIBS)

verify: $(VERIFY_TARGET)
	./$(VERIFY_TARGET) "$(OFFICIAL_KAT)"

clean:
	rm -f $(TARGET) $(VERIFY_TARGET)
	rm -rf output

.PHONY: all run verify clean
