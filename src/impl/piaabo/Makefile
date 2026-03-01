# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/piaabo
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))
# -----------------------------------------------------------------
# Build Rules (producers; no lib*.a prerequisites)
#   - Common utilities → build/common
#   - Security/encryption → build/openssl (needs SSL includes)
# -----------------------------------------------------------------
$(eval $(call BUILD_OBJ,      common,  dutils,         dutils))
$(eval $(call BUILD_OBJ,      common,  djson_parsing,  djson_parsing))
$(eval $(call BUILD_OBJ,      common,  dfiles,         dfiles))
$(eval $(call BUILD_OBJ,      common,  dconfig/config_space_t,    dconfig_config_space_t))
$(eval $(call BUILD_OBJ,      common,  dconfig/contract_space_t,  dconfig_contract_space_t))
$(eval $(call BUILD_OBJ_INC,  openssl, dencryption,    dencryption, $(SSL_INCLUDE_PATHS)))
$(eval $(call BUILD_OBJ_INC,  openssl, dsecurity,      dsecurity,   $(SSL_INCLUDE_PATHS)))

PIAABO := \
  $(OUTPUT_PATH)/common/dutils.o \
  $(OUTPUT_PATH)/common/djson_parsing.o \
  $(OUTPUT_PATH)/common/dfiles.o \
  $(OUTPUT_PATH)/common/dconfig/config_space_t.o \
  $(OUTPUT_PATH)/common/dconfig/contract_space_t.o \
  $(OUTPUT_PATH)/openssl/dencryption.o \
  $(OUTPUT_PATH)/openssl/dsecurity.o

.PHONY: dconfig_legacy_cleanup
dconfig_legacy_cleanup:
	@shred -u \
	  $(OUTPUT_PATH)/common/dconfig.o \
	  $(OUTPUT_PATH)/.deps/common/dconfig.d \
	  2>/dev/null || true
	@ar d $(OUTPUT_PATH)/libcommon.a dconfig.o 2>/dev/null || true

# -----------------------------------------------------------------
# Aggregate Target
#   - Build local piaabo objects
#   - Then recurse into https_compat, bnf_compat, math_compat and torch_compat
# -----------------------------------------------------------------
.PHONY: all
all: dconfig_legacy_cleanup $(PIAABO)
	$(MAKE) -C $(HERE_PATH)/https_compat all
	$(MAKE) -C $(HERE_PATH)/bnf_compat all
	$(MAKE) -C $(HERE_PATH)/math_compat all
	$(MAKE) -C $(HERE_PATH)/torch_compat all
	@$(LOG_SUCCESS)
