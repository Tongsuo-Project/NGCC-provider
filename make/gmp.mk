# GMP is needed only by the instances marked in make/ref.mk.
GMP_LIBRARY :=
ifneq ($(strip $(GMP_OBJECTS)),)
ifneq ($(GMP_SOURCE),)
GMP_BUILD_DIR := $(abspath build/gmp)
GMP_LIBRARY := $(GMP_BUILD_DIR)/.libs/libgmp.a
GMP_HEADER := $(GMP_BUILD_DIR)/gmp.h
GMP_HOST ?= $(if $(filter Darwin,$(PLATFORM)),$(if $(filter arm64,$(ARCH)),aarch64,x86_64)-apple-darwin,$(ARCH)-linux-gnu)
CPPFLAGS += -I$(GMP_BUILD_DIR)

# Reconfigure on source-directory, toolchain or flag changes, not on every make.
build/gmp-config: FORCE
	@mkdir -p build
	@printf '%s\n' $(foreach v,GMP_SOURCE GMP_HOST CC AR RANLIB CFLAGS LDFLAGS \
	    TARGET_FLAGS,'$(subst ','"'"',$($(v)))') > $@.tmp
	@cmp -s $@.tmp $@ && rm -f $@.tmp || mv -f $@.tmp $@

$(GMP_BUILD_DIR)/Makefile: build/gmp-config $(GMP_SOURCE)/configure make/gmp.mk
	rm -rf $(GMP_BUILD_DIR)
	mkdir -p $(GMP_BUILD_DIR)
	cd $(GMP_BUILD_DIR) && $(GMP_SOURCE)/configure \
	    --build="$$($(GMP_SOURCE)/config.guess)" --host="$(GMP_HOST)" \
	    --disable-shared --enable-static --with-pic --disable-cxx \
	    CC="$(CC)" AR="$(AR)" RANLIB="$(RANLIB)" \
	    CFLAGS="$(CFLAGS) $(TARGET_FLAGS)" LDFLAGS="$(LDFLAGS) $(TARGET_FLAGS)"

# Let GMP track its own source dependencies and preserve the archive timestamp
# when nothing changed. The all target also generates GMP's internal headers.
$(GMP_LIBRARY): $(GMP_BUILD_DIR)/Makefile FORCE
	+$(MAKE) -C $(GMP_BUILD_DIR) all

$(GMP_HEADER): $(GMP_LIBRARY)
	@test -f $@
else
GMP_STATIC ?= $(GMP_ROOT)/lib/libgmp.a
GMP_LIBRARY := $(GMP_STATIC)
GMP_HEADER := $(GMP_ROOT)/include/gmp.h
CPPFLAGS += -I$(GMP_ROOT)/include
endif

$(GMP_OBJECTS): $(GMP_HEADER)
endif
