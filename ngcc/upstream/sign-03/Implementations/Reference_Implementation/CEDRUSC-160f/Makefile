CC = gcc

PARAMS ?= cedrusc-sm3-160f
THASH ?= simple


OPTFLAGS ?= -O3 -fomit-frame-pointer -funroll-loops
WARNFLAGS ?= -Wall -Wextra
CFLAGS = $(OPTFLAGS) $(WARNFLAGS) -flto -DPARAMS=$(PARAMS)

COMMON_SRCS = address.c \
              auxfunc.c \
              counters.c \
              drng.c \
              hash_sm3.c \
              merkle.c \
              fors.c \
              randombytes.c \
              SIG_AlgorithmInstance.c \
              thash_sm3_$(THASH).c \
              utils.c \
              utilsx1.c \
              wots.c \
              wotsx1.c

COMMON_OBJS = $(COMMON_SRCS:.c=.o)

HEADERS = params.h \
          SIG_AlgorithmInstance.h \
          params/params-$(PARAMS).h

.PHONY: all clean show-config

all: KAT_SIG

show-config:
	@echo "PARAMS=$(PARAMS)"
	@echo "THASH=$(THASH)"
	@echo "ALGORITHM_INSTANCE=$(ALGORITHM_INSTANCE)"
	@echo "HASH=SM3"

KAT_SIG: KAT_SIG.o sign.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o KAT_SIG
