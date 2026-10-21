# Common rules for the fixed author ref instances in make/schemes/*.mk.
REF_CFLAGS := -fPIC -fcommon -fno-strict-aliasing -fwrapv -D_GNU_SOURCE \
              -Wno-error=implicit-function-declaration \
              -Wno-error=incompatible-pointer-types -Wno-error=int-conversion \
              -Wno-error=implicit-int -Wno-unused-function -Wno-unused-variable \
              -Wno-deprecated-declarations -std=gnu11
ifeq ($(PLATFORM),Darwin)
DARWIN_SDK := $(shell xcrun --show-sdk-path)
endif

# Arguments: object, source. Instance-specific flags belong to the target.
define ref_source
$(1): $(2) make/ref.mk $(_recipe) build/config
	@mkdir -p $$(@D)
	$$(CC) $$(CPPFLAGS) $$(CFLAGS) $$(TARGET_FLAGS) $$(REF_CFLAGS) $$(INSTANCE_FLAGS) \
	    $$(INSTANCE_INCLUDES) $$(SOURCE_FLAGS) -MMD -MP -c $$< -o $$@
endef

NGCC_CANDIDATES ?= ngcc/candidates
NGCC_API ?= ngcc/api

# Arguments: proposal ID, parameter label, kem/sig, strength, EVP name, getter.
# SRCDIR/SRCS/INC/DEFS/CFLAGS/SHIMDEFS/NOAUX are set by each frozen recipe.
define ngcc_instance
$(if $(or $(filter $(5),$(ENABLED_ALGORITHMS)),$(filter $(1)/$(2),$(CANDIDATE))),$(call ngcc_instance_rules,$(1),$(2),$(3),$(4),$(5),$(6)))
endef

define ngcc_instance_rules
$(eval _recipe := $(lastword $(MAKEFILE_LIST)))
$(eval _scheme := $(NGCC_CANDIDATES)/$(1))
$(eval _source := $(_scheme)/src/$(2))
$(eval _out := build/ngcc/$(1)/$(2))
$(eval _objects := $(foreach s,$(SRCS),$(_out)/$(subst /,_,$(s)).o))
$(eval _flags := $(INSTANCE_CFLAGS) $(DEFS))
$(if $(filter arm64 aarch64,$(ARCH)),$(eval _flags := $(subst -DTARGET_AMD64,-DTARGET_ARM64,$(_flags))))
$(if $(and $(filter arm64 aarch64,$(ARCH)),$(filter kem-01 kem-17 sign-15 sign-34,$(1))),$(eval _flags += -D__IMMINTRIN_H -include stddef.h))
$(if $(and $(filter Darwin,$(PLATFORM)),$(filter kem-02,$(1))),$(eval _flags += -I$(DARWIN_SDK)/usr/include/malloc -O3))
$(foreach s,$(SRCS),$(eval $(call ref_source,$(_out)/$(subst /,_,$(s)).o,$(_source)/$(s))))
$(if $(filter drng.c %/drng.c,$(SRCS)),,$(eval _objects += $(_out)/api_drng.o)$(eval $(call ref_source,$(_out)/api_drng.o,$(NGCC_API)/drng.c)))
$(if $(or $(filter auxfunc.c %/auxfunc.c,$(SRCS)),$(filter 1,$(NOAUX))),,$(eval _objects += $(_out)/api_auxfunc.o)$(eval $(call ref_source,$(_out)/api_auxfunc.o,$(NGCC_API)/auxfunc.c)))
$(eval _objects += $(_out)/shim.o $(_out)/ref.o)
$(eval $(call ref_source,$(_out)/shim.o,$(NGCC_API)/link_shim.c))
$(eval $(call ref_source,$(_out)/ref.o,src/ngcc_$(if $(filter kem,$(3)),kem,sig)_ref.c))

REF_INSTANCES += $(1)/$(2)
REF_OBJECTS += $(_out)/instance.o
REF_AUTHOR_SOURCES += $(addprefix $(_source)/,$(SRCS))
REF_DEPFILES += $(_objects:.o=.d)
$(if $(filter kem-12 kem-31 sign-25 sign-26 sign-27,$(1)),$(eval GMP_OBJECTS += $(_objects)))
$(_objects): INSTANCE_FLAGS := $(_flags)
$(_objects): INSTANCE_INCLUDES := -I$(_source) -I$(NGCC_API) $(addprefix -I$(_scheme)/,$(patsubst -I%,%,$(INC)))
$(_out)/shim.o: SOURCE_FLAGS := $(SHIMDEFS) -DNGCC_BUILD_$(if $(filter kem,$(3)),KEM,SIG) \
    '-DNGCC_ID="$(1)"' '-DNGCC_ALG="$(ALGORITHM)"' \
    '-DNGCC_SRCDIR="$(SRCDIR)"' '-DNGCC_VARIANT="Reference_Implementation"'
$(_out)/ref.o: SOURCE_FLAGS := -Isrc -DNGCC_REF_SYMBOL=$(6) \
    '-DNGCC_REF_ALGORITHM="$(5)"' '-DNGCC_REF_PROPOSAL="$(1)"' \
    '-DNGCC_REF_INSTANCE="$(2)"' -DNGCC_REF_SECURITY_BITS=$(4)

# Short relative symlinks keep spaces in author directory names out of rules.
$(_source):
	mkdir -p $$(@D)
	ln -s '../$(SRCDIR)' $$@
$(addprefix $(_source)/,$(SRCS)): | $(_source)
$(_objects): | $(_source)

$(_out)/instance.o: $(_objects)
	$$(LD) $$(if $$(filter Darwin,$$(PLATFORM)),$$(TARGET_FLAGS)) -r -o $$@.tmp $$^
ifeq ($(PLATFORM),Darwin)
	@printf '%s\n' '_$(6)' > $$(@D)/exports.txt
	$$(NMEDIT) -s $$(@D)/exports.txt $$@.tmp
else
	$$(OBJCOPY) --keep-global-symbol=$(6) $$@.tmp
endif
	mv -f $$@.tmp $$@
endef
