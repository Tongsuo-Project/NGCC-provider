CC ?= cc
AR ?= ar

CFLAGS ?= -std=c99 -O2
OFFICIAL_CFLAGS ?= $(CFLAGS)
LDFLAGS ?=
LDLIBS ?=
LOCAL_WARNFLAGS ?= -Wall -Wextra -Wpedantic -Wno-newline-eof -Wno-strict-prototypes
OFFICIAL_WARNFLAGS ?= -Wall -Wextra -Wno-newline-eof -Wno-strict-prototypes
STRICT_WARNFLAGS ?= -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -Wno-newline-eof -Wno-strict-prototypes
UBSAN_FLAGS ?= -O1 -g -fsanitize=undefined -fno-omit-frame-pointer
ASAN_FLAGS ?= -O1 -g -fsanitize=address -fno-omit-frame-pointer
CPPFLAGS ?= -Isrc -Ilib/api_pkc
RBC_DIR := lib/rbc-47
RBC_CPPFLAGS := -I$(RBC_DIR)

BUILD_DIR := build
BIN_DIR := bin
TARGET := $(BIN_DIR)/KAT_KEM

SRCS := \
	$(RBC_DIR)/rbc_elt.c \
	$(RBC_DIR)/rbc_vec.c \
	src/KEM_AlgorithmInstance.c \
	src/augabidulin.c \
	src/ct_util.c \
	src/gabidulin.c \
	src/loong_api_random.c \
	src/loong_hash.c \
	src/loong_kem.c \
	src/loong_pke.c \
	src/loong_support.c \
	src/loong_xof_reader.c \
	src/parsing.c \
	src/qpoly.c \
	lib/api_pkc/auxfunc.c \
	lib/api_pkc/drng.c \
	test/KAT_KEM.c

OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))
CFLAGS_USED = $(CFLAGS)
WARNFLAGS_USED = $(LOCAL_WARNFLAGS)
INSTANCE_DIR := $(notdir $(CURDIR))

.PHONY: all asan bench clean distclean kat sanitizer strict submission-check symbols test ubsan

all: $(TARGET)

$(TARGET): $(OBJS)
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USED) $(WARNFLAGS_USED) $(CPPFLAGS) $(RBC_CPPFLAGS) -c $< -o $@

$(BUILD_DIR)/lib/api_pkc/%.o: CFLAGS_USED = $(OFFICIAL_CFLAGS)
$(BUILD_DIR)/lib/api_pkc/%.o: WARNFLAGS_USED = $(OFFICIAL_WARNFLAGS)
$(BUILD_DIR)/test/KAT_KEM.o: CFLAGS_USED = $(OFFICIAL_CFLAGS)
$(BUILD_DIR)/test/%.o: WARNFLAGS_USED = $(OFFICIAL_WARNFLAGS)

symbols: $(TARGET)
	nm $(TARGET)

test: kat

kat: $(TARGET)
	$(TARGET)

bench: kat

submission-check:
	$(MAKE) distclean
	$(MAKE) kat
	$(MAKE) distclean

strict:
	$(MAKE) clean
	$(MAKE) LOCAL_WARNFLAGS="$(STRICT_WARNFLAGS)" all

sanitizer: ubsan

ubsan:
	$(MAKE) clean
	$(MAKE) CFLAGS="-std=c99 $(UBSAN_FLAGS)" OFFICIAL_CFLAGS="-std=c99 -O2" LDFLAGS="$(UBSAN_FLAGS)" all
	$(TARGET)

asan:
	$(MAKE) clean
	ASAN_OPTIONS=detect_leaks=0 $(MAKE) CFLAGS="-std=c99 $(ASAN_FLAGS)" OFFICIAL_CFLAGS="-std=c99 -O2" LDFLAGS="$(ASAN_FLAGS)" all
	ASAN_OPTIONS=detect_leaks=0 $(TARGET)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

distclean: clean
	rm -rf output
