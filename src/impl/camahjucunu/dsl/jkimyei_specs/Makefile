# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../../../../
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/camahjucunu/dsl/jkimyei_specs
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))
# -----------------------------------------------------------------
# Build Rules
# -----------------------------------------------------------------
$(eval $(call BUILD_OBJ, common, jkimyei_specs, dsl_jkimyei_specs))

# -----------------------------------------------------------------
# Aggregate Target
# -----------------------------------------------------------------
.PHONY: all
all: dsl_jkimyei_specs
	@$(LOG_SUCCESS)
