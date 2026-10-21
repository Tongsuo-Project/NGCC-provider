CC := gcc
AR := ar

CFLAGS := -m64 -O3 -std=c99
CPPFLAGS := -I. -DNIST_RAND=1
LDLIBS := -lcrypto -lm
PARAM_DEFS :=
SECURITY ?= 128

ifeq ($(SECURITY),128)
SECURITY_DEFS := -DBIKE_SECURITY_128 -DBIKE_MLTHRE_ENABLED=1
else ifeq ($(SECURITY),256)
SECURITY_DEFS := -DBIKE_SECURITY_256 -DBIKE_MLTHRE_ENABLED=1
else ifeq ($(SECURITY),512)
SECURITY_DEFS := -DBIKE_SECURITY_512 -DBIKE_MLTHRE_ENABLED=1
else ifeq ($(SECURITY),192)
SECURITY_DEFS := -DBIKE_SECURITY_192 -DBIKE_MLTHRE_ENABLED=1
else
$(error Unsupported SECURITY=$(SECURITY). Use 128, 192, 256 or 512)
endif

ALL_DEFS := $(PARAM_DEFS) $(SECURITY_DEFS)

PUBLIC_LIB_TARGET := libbike_v2.a
BUILD_DIR := .build/$(SECURITY)
LIB_TARGET := $(BUILD_DIR)/libbike_v2.a
KAT_TARGET := kat_bike_kem_$(SECURITY)
KAT_SRC := kat/KAT_KEM.c \
           kat/KEM_AlgorithmInstance.c \
           kat/drng.c

SRC := auxfunc.c \
       conversions.c \
       decode_ml.c \
       hash_wrapper.c \
       kem.c \
       mlthre_model.c \
       mlthre_policy.c \
       mlthre_runtime_sampling.c \
       ntl.c \
       ring_buffer.c \
       sampling.c \
       threshold.c \
       utilities.c \
       xof_prng.c \
       FromNIST/rng.c

OBJ := $(addprefix $(BUILD_DIR)/,$(SRC:.c=.o))
LEGACY_OBJ := $(SRC:.c=.o)

.PHONY: all clean distclean kat run-kat kat128 kat192 kat256 kat512

all: $(LIB_TARGET)
	cp $(LIB_TARGET) $(PUBLIC_LIB_TARGET)

$(LIB_TARGET): $(OBJ)
	$(AR) rcs $@ $(OBJ)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(ALL_DEFS) -c $< -o $@

$(KAT_TARGET): $(LIB_TARGET) $(KAT_SRC) kat/KEM_AlgorithmInstance.h kat/drng.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(ALL_DEFS) -Ikat $(KAT_SRC) $(LIB_TARGET) $(LDLIBS) -o $@

kat: run-kat

run-kat: all $(KAT_TARGET)
	./$(KAT_TARGET)

kat128:
	$(MAKE) clean
	$(MAKE) run-kat SECURITY=128

kat192:
	$(MAKE) clean
	$(MAKE) run-kat SECURITY=192

kat256:
	$(MAKE) clean
	$(MAKE) run-kat SECURITY=256

kat512:
	$(MAKE) clean
	$(MAKE) run-kat SECURITY=512

clean:
	rm -rf .build
	rm -f $(LEGACY_OBJ) $(PUBLIC_LIB_TARGET) kat_bike_kem_128 kat_bike_kem_192 kat_bike_kem_256 kat_bike_kem_512

distclean: clean
	rm -rf output
