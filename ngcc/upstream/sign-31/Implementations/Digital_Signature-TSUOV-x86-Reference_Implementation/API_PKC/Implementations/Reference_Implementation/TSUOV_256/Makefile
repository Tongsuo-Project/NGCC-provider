CC      = gcc
VARIANT ?= 256
ifdef TSUOV_VARIANT
  VARIANT := $(TSUOV_VARIANT)
endif
CFLAGS  = -std=c99 -Wpedantic -Wall -Wextra -O2 -DTSUOV_VARIANT=$(VARIANT) -I.
LDFLAGS = -lm

CORE_SRCS  = tsuov_core.c mgf.c Fql.c
LOCAL_SRCS = SIG_AlgorithmInstance.c auxfunc.c drng.c KAT_SIG.c

.PHONY: all kat clean

all: kat

KAT_SIG: $(LOCAL_SRCS) $(CORE_SRCS)
	$(CC) $(CFLAGS) -o $@ $(LOCAL_SRCS) $(CORE_SRCS) $(LDFLAGS)

kat: KAT_SIG
	./KAT_SIG

clean:
	rm -f KAT_SIG
