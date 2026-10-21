ifeq ($(origin CC),default)
CC := gcc
endif
BUILD ?= release
.DEFAULT_GOAL := test

VALID_BUILDS := debug release
ifeq (,$(filter $(BUILD),$(VALID_BUILDS)))
$(error Invalid BUILD='$(BUILD)'. Expected one of: $(VALID_BUILDS))
endif

PROJECT_ROOT := $(abspath ..)
SQISIGN_DIR := $(PROJECT_ROOT)/sqisign
EC_ARITH_DIR := $(PROJECT_ROOT)/ec_arithmetic

BUILD_DIR := build/$(BUILD)
OBJDIR := $(BUILD_DIR)/obj
BINDIR := $(BUILD_DIR)/bin

TEST_TARGET := test_sqisigndim2
KAT_TARGET  := kat_sig

TEST_BIN := $(BINDIR)/$(TEST_TARGET)
KAT_BIN  := $(BINDIR)/$(KAT_TARGET)

INCLUDES := -I$(SQISIGN_DIR) -I$(PROJECT_ROOT) -I$(EC_ARITH_DIR)/include

GMP_PREFIX ?=
ifneq ($(strip $(GMP_PREFIX)),)
INCLUDES += -I$(GMP_PREFIX)/include
LDFLAGS += -L$(GMP_PREFIX)/lib
endif

COMMON_CFLAGS := -std=c99 -Wpedantic -Wall -Wextra -MMD -MP
DEBUG_CFLAGS := -O0 -g
RELEASE_CFLAGS := -O2 -DNDEBUG

ifeq ($(BUILD),debug)
CFLAGS += $(COMMON_CFLAGS) $(DEBUG_CFLAGS)
else
CFLAGS += $(COMMON_CFLAGS) $(RELEASE_CFLAGS)
endif
ifneq (,$(findstring -DCOMPRESSED,$(EXTRA_CFLAGS)))
$(error Select the compression mode in ../config.h, not through EXTRA_CFLAGS)
endif
CFLAGS += $(EXTRA_CFLAGS)

LDLIBS += -lgmp

CONFIG_HASH := $(shell sha256sum $(PROJECT_ROOT)/config.h | cut -d' ' -f1)
CONFIG_STAMP := $(BUILD_DIR)/.config-$(shell printf '%s' '$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) $(LDLIBS) $(CONFIG_HASH)' | sha256sum | cut -d' ' -f1)

CORE_SRCS := \
    $(PROJECT_ROOT)/algebra.c \
    $(PROJECT_ROOT)/auxfunc.c \
    $(PROJECT_ROOT)/basis.c \
    $(PROJECT_ROOT)/biextension.c \
    $(PROJECT_ROOT)/dim2.c \
    $(PROJECT_ROOT)/dim2id2iso.c \
    $(PROJECT_ROOT)/dim4.c \
    $(PROJECT_ROOT)/drng.c \
    $(PROJECT_ROOT)/ec.c \
    $(PROJECT_ROOT)/finit.c \
    $(PROJECT_ROOT)/fp.c \
    $(PROJECT_ROOT)/fp2.c \
    $(PROJECT_ROOT)/hd.c \
    $(PROJECT_ROOT)/id2iso.c \
    $(PROJECT_ROOT)/ideal.c \
    $(PROJECT_ROOT)/intbig.c \
    $(PROJECT_ROOT)/integers.c \
    $(PROJECT_ROOT)/isog_chains.c \
    $(PROJECT_ROOT)/klptx.c \
    $(PROJECT_ROOT)/lattice.c \
    $(PROJECT_ROOT)/lll.c \
    $(PROJECT_ROOT)/matkermod.c \
    $(PROJECT_ROOT)/mp.c \
    $(PROJECT_ROOT)/pack.c \
    $(PROJECT_ROOT)/printer.c \
    $(PROJECT_ROOT)/theta_isogenies.c \
    $(PROJECT_ROOT)/theta_structure.c \
    $(PROJECT_ROOT)/tools.c \
    $(PROJECT_ROOT)/xeval.c \
    $(PROJECT_ROOT)/xisog.c

EC_ARITH_SRCS := $(wildcard $(EC_ARITH_DIR)/*.c)

SQISIGN_COMMON_SRCS := \
    $(SQISIGN_DIR)/keygen.c \
    $(SQISIGN_DIR)/sign.c \
    $(SQISIGN_DIR)/SIG_AlgorithmInstance.c

ASM_SRCS :=

TEST_MAIN_SRC := $(SQISIGN_DIR)/test_sqisigndim2.c
KAT_MAIN_SRC  := $(SQISIGN_DIR)/KAT_SIG.c

COMMON_SRCS := $(CORE_SRCS) $(EC_ARITH_SRCS) $(SQISIGN_COMMON_SRCS) $(ASM_SRCS)

TEST_SRCS := $(COMMON_SRCS) $(TEST_MAIN_SRC)
KAT_SRCS  := $(COMMON_SRCS) $(KAT_MAIN_SRC)

define make-objs
$(addprefix $(OBJDIR)/,$(patsubst $(PROJECT_ROOT)/%,%,$(patsubst %.c,%.o,$($(1)))))
endef

TEST_OBJS := $(call make-objs,TEST_SRCS)
KAT_OBJS  := $(call make-objs,KAT_SRCS)

TEST_DEPS := $(TEST_OBJS:.o=.d)
KAT_DEPS  := $(KAT_OBJS:.o=.d)

$(CONFIG_STAMP):
	@mkdir -p $(BUILD_DIR)
	@rm -f $(BUILD_DIR)/.config-*
	@touch $@

$(OBJDIR)/%.o: $(PROJECT_ROOT)/%.c $(CONFIG_STAMP)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

.PHONY: all test kat run run-kat clean

all: test

test: $(TEST_BIN)

kat: $(KAT_BIN)

$(TEST_BIN): $(TEST_OBJS) $(CONFIG_STAMP)
	@mkdir -p $(dir $@)
	$(CC) $(TEST_OBJS) $(LDFLAGS) $(LDLIBS) -o $@

$(KAT_BIN): $(KAT_OBJS) $(CONFIG_STAMP)
	@mkdir -p $(dir $@)
	$(CC) $(KAT_OBJS) $(LDFLAGS) $(LDLIBS) -o $@

$(OBJDIR)/%.o: $(PROJECT_ROOT)/%.c $(CONFIG_STAMP)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

run: $(TEST_BIN)
	./$(TEST_BIN)

run-kat: $(KAT_BIN)
	./$(KAT_BIN)

clean:
	$(RM) -r build output

-include $(TEST_DEPS)
-include $(KAT_DEPS)
