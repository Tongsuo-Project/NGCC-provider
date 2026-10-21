CC ?= cc

CFLAGS += -O3 -DNDEBUG -std=c11 -Wall -Wextra -Werror=implicit-function-declaration
CPPFLAGS += -I. -Isrc -Ilib -Ilib/blake2/ref \
	-DHAVE_SYS_RANDOM_H -DHAVE_GETRANDOM -DSYDO_REF_NGCC
CFLAGS += $(CFLAGS_EXTRA)
CPPFLAGS += $(CPPFLAGS_EXTRA)

COMMON_SOURCES = \
	SIG_AlgorithmInstance.c \
	src/aes.c \
	src/bavc.c \
	src/compat.c \
	src/fields.c \
	src/randomness.c \
	src/sydo.c \
	src/field.c \
	src/hash.c \
	src/macs.c \
	src/primitives.c \
	src/quicksilver.c \
	src/rsd.c \
	src/vole_check.c \
	src/universal_hashing.c \
	src/utils.c \
	src/vole.c \
	lib/auxfunc.c \
	lib/drng.c \
	lib/blake2/ref/blake2b-ref.c

COMMON_OBJECTS = $(COMMON_SOURCES:.c=.o)

.PHONY: all clean

all: kat_sig bench

kat_sig: test/KAT_SIG.o $(COMMON_OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

bench: test/bench.o $(COMMON_OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@


clean:
	rm -f $(COMMON_OBJECTS) test/KAT_SIG.o test/bench.o kat_sig bench
	rm -rf output
