# QingLuan-128 reference instance - self-contained.
# Security level is fixed in include/params.h; no -DQINGLUAN_xxx needed.
CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c11 -O2
INCLUDES = -Iinclude
LDFLAGS =
ifeq ($(OS),Windows_NT)
    LDFLAGS += -lbcrypt
    EXE_EXT = .exe
endif
SRCS = src/fq_arith.c src/hash.c src/utils.c src/restr.c src/rsdp.c        src/mpc.c src/keygen.c src/sign.c src/verify.c src/selftest.c
all: qingluan_test$(EXE_EXT) qingluan_kat$(EXE_EXT)
qingluan_test$(EXE_EXT): $(SRCS) test/main.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LDFLAGS)
qingluan_kat$(EXE_EXT): $(SRCS) test/kat_gen.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LDFLAGS)
clean:
	rm -f qingluan_test$(EXE_EXT) qingluan_kat$(EXE_EXT)
.PHONY: all clean
