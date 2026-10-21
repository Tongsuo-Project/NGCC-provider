CC = gcc

PARAMS ?= flextree-160s
THASH ?= simple


OPTFLAGS ?= -O3 -funroll-loops
WARNFLAGS ?= -std=c99 -Wpedantic -Wall -Wextra
CFLAGS = $(OPTFLAGS) $(WARNFLAGS) -DPARAMS=$(PARAMS)
LIBS =

COMMON_SRCS = address.c \
              auxfunc.c \
              counters.c \
              drng.c \
              hash_sm3.c \
              merkle.c \
              pors_fp.c \
              randombytes.c \
              SIG_AlgorithmInstance.c \
              thash_sm3_$(THASH).c \
              utils.c \
              utilsx1.c \
              wots.c \
              wotsx1.c

COMMON_OBJS = $(COMMON_SRCS:.c=.o)

HEADERS = params.h \
          pors_precomputed.h \
          SIG_AlgorithmInstance.h \
          params/params-$(PARAMS).h \
          params/pors_precomputed-$(PARAMS).h

.PHONY: all clean show-config

all: KAT_SIG

show-config:
	@echo "PARAMS=$(PARAMS)"
	@echo "THASH=$(THASH)"
	@echo "ALGORITHM_INSTANCE is defined in SIG_AlgorithmInstance.h"
	@echo "HASH=SM3"

KAT_SIG: KAT_SIG.o sign.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o KAT_SIG
