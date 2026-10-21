CC = gcc
CPPFLAGS ?= -I. -DBWKEM128_INTERNAL_COMPAT -DBWKEM128_USE_ICCS_AUXFUNC
CFLAGS ?= -std=c99 -Wpedantic -Wall -Wextra -O2
LDFLAGS ?=
LDLIBS ?=

BUILD_DIR := build
TEST_DIR := ../../test
KAT_TARGET := ../../../Test_Vector/KAT_KEM_BW_KEM_C128.txt
KAT_LOCAL := $(BUILD_DIR)/output/KAT_KEM_BW_KEM_C128.txt

CORE_SRC := 	BWcoding.c 	cbd.c 	indcpa.c 	kem.c 	ntt.c 	poly.c 	polyvec.c 	randombytes_api.c 	reduce.c 	symmetric-iccs.c 	verify.c

API_COMMON_SRC := 	auxfunc.c 	drng.c 	KEM_AlgorithmInstance.c

KAT_SRC := KAT_KEM.c

COMMON_OBJS := $(addprefix $(BUILD_DIR)/,$(CORE_SRC:.c=.o)) 	$(addprefix $(BUILD_DIR)/,$(API_COMMON_SRC:.c=.o))
KAT_OBJS := $(COMMON_OBJS) $(addprefix $(BUILD_DIR)/,$(KAT_SRC:.c=.o))

.PHONY: all kat speed selfeval clean

all: $(BUILD_DIR)/kat_kem

kat: $(BUILD_DIR)/kat_kem
	cd $(BUILD_DIR) && ./kat_kem
	cp $(KAT_LOCAL) $(KAT_TARGET)
	$(MAKE) clean

speed: $(BUILD_DIR)/test_speed
	./$(BUILD_DIR)/test_speed

selfeval: $(BUILD_DIR)/selfeval
	./$(BUILD_DIR)/selfeval

$(BUILD_DIR)/selfeval: $(COMMON_OBJS) $(TEST_DIR)/selfeval_kem.c $(TEST_DIR)/cpucycles.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $(COMMON_OBJS) $(TEST_DIR)/selfeval_kem.c $(TEST_DIR)/cpucycles.c $(LDLIBS) -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/kat_kem: $(KAT_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(KAT_OBJS) $(LDLIBS) -o $@

$(BUILD_DIR)/test_speed: $(COMMON_OBJS) test_speed.c $(TEST_DIR)/cpucycles.c $(TEST_DIR)/speed_print.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $(COMMON_OBJS) test_speed.c $(TEST_DIR)/cpucycles.c $(TEST_DIR)/speed_print.c $(LDLIBS) -o $@

clean:
	rm -rf $(BUILD_DIR)
