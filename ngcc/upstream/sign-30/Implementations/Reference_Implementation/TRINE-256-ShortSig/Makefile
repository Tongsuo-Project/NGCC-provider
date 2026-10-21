SHELL := /bin/bash

CC ?= gcc
PARAMS ?= 1
LIBS = -lssl -lcrypto

ifeq ($(PARAMS),1)
TRINE_ALGORITHM_INSTANCE := MEDS2endGen-Balanced-I
else ifeq ($(PARAMS),2)
TRINE_ALGORITHM_INSTANCE := MEDS2endGen-Balanced-III
else ifeq ($(PARAMS),3)
TRINE_ALGORITHM_INSTANCE := MEDS2endGen-Balanced-V
else ifeq ($(PARAMS),4)
TRINE_ALGORITHM_INSTANCE := MEDS2endGen-ShortSig-I
else ifeq ($(PARAMS),5)
TRINE_ALGORITHM_INSTANCE := MEDS2endGen-ShortSig-III
else ifeq ($(PARAMS),6)
TRINE_ALGORITHM_INSTANCE := MEDS2endGen-ShortSig-V
else
$(error PARAMS must be in the range 1..6)
endif

ifdef DEBUG
COMMON_CFLAGS := -g -Wall -DDEBUG
else
COMMON_CFLAGS := -O3 -Wall
endif

ifdef LOG
COMMON_CFLAGS += -DDEBUG
endif

COMMON_CFLAGS += -DPARAMS=$(PARAMS) -I.
NORMAL_CFLAGS := $(COMMON_CFLAGS) -DUSE_SHA3
ICCS_CFLAGS := $(COMMON_CFLAGS) -DUSE_ICCS -Iiccs \
	-DTRINE_ALGORITHM_INSTANCE=\"$(TRINE_ALGORITHM_INSTANCE)\"

NORMAL_OBJDIR := build/p$(PARAMS)/sha3
ICCS_OBJDIR := build/p$(PARAMS)/iccs

CORE_OBJECT_NAMES := meds.o util.o osfreq.o field.o matrixmod.o matrixelim.o \
	triform.o canonical.o corank1.o trine_expand.o trine_codec.o bitstream.o \
	hashkdf.o

NORMAL_OBJECTS := $(addprefix $(NORMAL_OBJDIR)/,$(CORE_OBJECT_NAMES) fips202.o randombytes.o)
ICCS_OBJECTS := $(addprefix $(ICCS_OBJDIR)/,$(CORE_OBJECT_NAMES) randombytes_iccs.o)

SPEED_EXE := $(NORMAL_OBJDIR)/speed
KAT_EXE := $(ICCS_OBJDIR)/KAT_SIG

.PHONY: all clean KAT speed

all:
	for p in 1 2 3 4 5 6; do $(MAKE) KAT speed PARAMS=$$p || exit $$?; done

$(NORMAL_OBJDIR) $(ICCS_OBJDIR):
	mkdir -p $@

$(NORMAL_OBJDIR)/%.o: %.c $(wildcard *.h) | $(NORMAL_OBJDIR)
	$(CC) $(NORMAL_CFLAGS) -c $< -o $@

$(ICCS_OBJDIR)/%.o: %.c $(wildcard *.h) $(wildcard iccs/*.h) | $(ICCS_OBJDIR)
	$(CC) $(ICCS_CFLAGS) -c $< -o $@

$(SPEED_EXE): test_speed/test_speed.c test_speed/cpucycles.c test_speed/speed_print.c $(NORMAL_OBJECTS)
	$(CC) test_speed/test_speed.c test_speed/cpucycles.c test_speed/speed_print.c \
		$(NORMAL_OBJECTS) $(NORMAL_CFLAGS) -Itest_speed $(LIBS) -o $@

$(KAT_EXE): $(ICCS_OBJECTS) iccs/KAT_SIG.c iccs/SIG_AlgorithmInstance.c iccs/drng.c iccs/auxfunc.c | $(ICCS_OBJDIR)
	$(CC) iccs/KAT_SIG.c iccs/SIG_AlgorithmInstance.c iccs/drng.c iccs/auxfunc.c \
		$(ICCS_OBJECTS) $(ICCS_CFLAGS) -o $@

KAT: $(KAT_EXE)

speed: $(SPEED_EXE)

clean:
	rm -rf build debug
