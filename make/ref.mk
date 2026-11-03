# Common rules for the pinned author reference and optimized implementations.
comma := ,
# Only options the provider genuinely needs to emit a shared object are added
# here.  Optimisation, tuning, instruction-set and language flags come from the
# author recipe of each instance (INSTANCE_CFLAGS in make/schemes/*.mk for the
# reference build, make/implementations.json for the optimized build).
REF_CFLAGS := -fPIC -fcommon -D_GNU_SOURCE \
              -Wno-error=implicit-function-declaration \
              -Wno-error=incompatible-pointer-types -Wno-error=int-conversion \
              -Wno-error=implicit-int -Wno-unused-function -Wno-unused-variable \
              -Wno-deprecated-declarations -std=gnu11
ifeq ($(PLATFORM),Darwin)
DARWIN_SDK := $(shell xcrun --show-sdk-path)
endif

# Arguments: object, source. Instance-specific flags belong to the target.
define ref_source
$(1): $(2) make/ref.mk $(_recipe) Makefile build/config
	@mkdir -p $$(@D)
	$(if $(filter %.cpp %.cc,$(2)),$$(CXX),$$(CC)) $$(CPPFLAGS) $$(TARGET_FLAGS) \
	    $(if $(filter %.cpp %.cc,$(2)),$$(filter-out -std=%,$$(REF_CFLAGS) $$(INSTANCE_FLAGS)) -std=c++2b -Imake/compat/cxx -include algorithm -include make/compat/cxx/alloc.h $$(CXXFLAGS),$$(REF_CFLAGS) $$(INSTANCE_FLAGS)) $(if $(filter %.S %.s,$(2)),-Wa$$(comma)-I$(_source)) \
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
$(eval _variant := $(if $(filter 1,$(REFERENCE_TEST)),ref,$(or $(OPT_VARIANT_$(5)),ref)))
$(foreach field,SRCDIR SRCS INC DEFS INSTANCE_CFLAGS SHIMDEFS NOAUX,$(if $(and $(OPT_VARIANT_$(5)),$(filter-out ref,$(_variant))),$(eval $(field) := $(OPT_$(field)_$(5)))))
$(eval _source := build/sources/$(1)/$(2)/$(_variant))
$(eval _includes := $(foreach i,$(INC),$(if $(filter -Isrc/$(2)%,$(i)),$(subst -Isrc/$(2),-I$(_source),$(i)),-I$(_scheme)/$(patsubst -I%,%,$(i)))))
$(eval _out := build/ngcc/$(TARGET)/$(1)/$(2)/$(_variant))
$(eval _objects := $(foreach s,$(SRCS),$(_out)/$(subst /,_,$(s)).o))
$(eval _flags := $(INSTANCE_CFLAGS) $(DEFS))
$(if $(filter arm64 aarch64,$(ARCH)),$(eval _flags := $(subst -DTARGET_AMD64,-DTARGET_ARM64,$(_flags))))
$(if $(and $(filter ref,$(_variant)),$(filter arm64 aarch64,$(ARCH)),$(filter kem-01 kem-17 sign-15 sign-34,$(1))),$(eval _flags += -D__IMMINTRIN_H -include stddef.h))
$(if $(and $(filter Darwin,$(PLATFORM)),$(filter kem-02,$(1))),$(eval _flags += -I$(DARWIN_SDK)/usr/include/malloc -O3))
$(foreach s,$(SRCS),$(eval $(call ref_source,$(_out)/$(subst /,_,$(s)).o,$(_source)/$(s))))
$(if $(filter-out ref,$(_variant)),$(foreach s,$(SRCS),$(eval $(_out)/$(subst /,_,$(s)).o: SOURCE_FLAGS := $(OPT_SOURCE_CFLAGS_$(5)_$(subst /,_,$(s))))))
$(if $(and $(filter Linux,$(PLATFORM)),$(filter kem-02,$(1))),$(eval $(_out)/backend_ntt_2s3t.c.o: SOURCE_FLAGS := -fgnu89-inline))
$(if $(filter sign-07,$(1)),$(eval $(_out)/ntt.c.o: SOURCE_FLAGS := -I. '-DNGCC_CS_NTT_HEADER="$(_source)/ntt.h"' -include make/compat/sign-07/optimized_ntt.h))
$(if $(filter-out ref,$(_variant)),$(foreach s,$(OPT_GLUE_SRCS_$(5)),$(eval _objects += $(_out)/$(notdir $(s)).o)$(eval $(call ref_source,$(_out)/$(notdir $(s)).o,$(s)))))
$(if $(filter drng.c %/drng.c,$(SRCS)),,$(eval _objects += $(_out)/api_drng.o)$(eval $(call ref_source,$(_out)/api_drng.o,$(NGCC_API)/drng.c)))
$(if $(or $(filter auxfunc.c %/auxfunc.c,$(SRCS)),$(filter 1,$(NOAUX))),,$(eval _objects += $(_out)/api_auxfunc.o)$(eval $(call ref_source,$(_out)/api_auxfunc.o,$(NGCC_API)/auxfunc.c)))
$(eval _objects += $(_out)/shim.o $(_out)/ref.o)
$(eval $(call ref_source,$(_out)/shim.o,$(NGCC_API)/link_shim.c))
$(eval $(call ref_source,$(_out)/ref.o,src/ngcc_$(if $(filter kem,$(3)),kem,sig)_ref.c))

REF_INSTANCES += $(1)/$(2)
REF_IMPLEMENTATIONS += $(5):$(_variant)
REF_OBJECTS += $(_out)/instance.o
REF_AUTHOR_SOURCES += $(addprefix $(_source)/,$(SRCS))
REF_AUTHOR_SOURCES += $(if $(filter-out ref,$(_variant)),$(OPT_GLUE_SRCS_$(5)))
REF_DEPFILES += $(_objects:.o=.d)
$(if $(filter kem-12 kem-31 sign-25 sign-26 sign-27,$(1)),$(eval GMP_OBJECTS += $(_objects)))
$(_objects): INSTANCE_FLAGS := $(_flags)
$(_objects): INSTANCE_INCLUDES := -I$(_source) -I$(NGCC_API) $(_includes)
$(_out)/shim.o: SOURCE_FLAGS := $(SHIMDEFS) -DNGCC_BUILD_$(if $(filter kem,$(3)),KEM,SIG) \
    '-DNGCC_ID="$(1)"' '-DNGCC_ALG="$(ALGORITHM)"' \
    '-DNGCC_SRCDIR="$(SRCDIR)"' '-DNGCC_VARIANT="$(_variant)"'
$(_out)/ref.o: SOURCE_FLAGS := -Isrc -DNGCC_REF_SYMBOL=$(6) \
    '-DNGCC_REF_ALGORITHM="$(5)"' '-DNGCC_REF_PROPOSAL="$(1)"' \
    '-DNGCC_REF_INSTANCE="$(2)"' -DNGCC_REF_SECURITY_BITS=$(4)

# Short relative symlinks keep spaces in author directory names out of rules.
$(_source): Makefile
	mkdir -p $$(@D)
	ln -sfn '$(abspath $(_scheme))/$(SRCDIR)' $$@
$(addprefix $(_source)/,$(SRCS)): | $(_source)
$(_objects): | $(_source)

$(_out)/instance.o: $(_objects)
	$$(LD) $$(if $$(filter Darwin,$$(PLATFORM)),$$(TARGET_FLAGS) -d) -r -o $$@.tmp $$^
ifeq ($(PLATFORM),Darwin)
	@printf '%s\n' '_$(6)' > $$(@D)/exports.txt
	$$(NMEDIT) -s $$(@D)/exports.txt $$@.tmp
	@$$(NM) -gU $$@.tmp > $$(@D)/symbols.txt
	@awk '$$$$NF != "_$(6)" { print "Unexpected instance symbol: " $$$$0; bad = 1 } END { exit bad }' $$(@D)/symbols.txt
else
	@# objcopy cannot localize COMMON symbols; give each instance its own names.
	@$$(NM) -g --defined-only $$@.tmp > $$(@D)/symbols.txt
	@awk '$$$$2 == "C" { print $$$$3 " $(6)_common_" $$$$3 }' $$(@D)/symbols.txt > $$(@D)/common-symbols.txt
	@awk '$$$$2 ~ /^[TDBRGS]$$$$/ && $$$$3 != "$(6)" { print $$$$3 }' $$(@D)/symbols.txt > $$(@D)/localize-symbols.txt
	$$(OBJCOPY) --redefine-syms=$$(@D)/common-symbols.txt --localize-symbols=$$(@D)/localize-symbols.txt $$@.tmp
	@$$(NM) -g --defined-only $$@.tmp > $$(@D)/symbols.txt
	@awk '$$$$3 == "$(6)" { next } $$$$2 == "C" && index($$$$3, "$(6)_common_") == 1 { next } $$$$2 ~ /^[WwVvu]$$$$/ { next } { print "Unexpected instance symbol: " $$$$0; bad = 1 } END { exit bad }' $$(@D)/symbols.txt
endif
	mv -f $$@.tmp $$@
endef
