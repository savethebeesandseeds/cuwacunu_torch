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
$(eval $(call BUILD_OBJ,      common,  latent_lineage_state/runtime_lls,    runtime_lls))
$(eval $(call BUILD_OBJ_INC,  openssl, dencryption,    dencryption, $(SSL_INCLUDE_PATHS)))
$(eval $(call BUILD_OBJ_INC,  openssl, dsecurity,      dsecurity,   $(SSL_INCLUDE_PATHS)))

PIAABO_CORE := \
  dutils \
  djson_parsing \
  dfiles \
  runtime_lls \
  dencryption \
  dsecurity

# -----------------------------------------------------------------
# Aggregate Target
#   - Build local piaabo leaf producers.
#   - Fan out into https_compat, bnf_compat, math_compat and torch_compat so
#     GNU make can schedule them in parallel.
# -----------------------------------------------------------------
.PHONY: https_compat
https_compat:
	$(MAKE) -C $(HERE_PATH)/https_compat all

.PHONY: bnf_compat
bnf_compat:
	$(MAKE) -C $(HERE_PATH)/bnf_compat all

.PHONY: math_compat
math_compat:
	$(MAKE) -C $(HERE_PATH)/math_compat all

.PHONY: torch_compat
torch_compat:
	$(MAKE) -C $(HERE_PATH)/torch_compat all

.PHONY: all
all: $(PIAABO_CORE) https_compat bnf_compat math_compat torch_compat
	@$(LOG_SUCCESS)
