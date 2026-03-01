# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../../../../
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/camahjucunu/dsl/tsiemene_wave
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))

# -----------------------------------------------------------------
# Build Rules
# -----------------------------------------------------------------
$(eval $(call BUILD_OBJ, common, tsiemene_wave, dsl_tsiemene_wave))

# -----------------------------------------------------------------
# Aggregate Target
# -----------------------------------------------------------------
.PHONY: all
all: dsl_tsiemene_wave
	@$(LOG_SUCCESS)
