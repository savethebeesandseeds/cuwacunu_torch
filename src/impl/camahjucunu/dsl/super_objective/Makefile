# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../../../../
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/camahjucunu/dsl/super_objective
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))

# -----------------------------------------------------------------
# Build Rules
# -----------------------------------------------------------------
$(eval $(call BUILD_OBJ, common, super_objective, dsl_super_objective))

# -----------------------------------------------------------------
# Aggregate Target
# -----------------------------------------------------------------
.PHONY: all
all: dsl_super_objective
	@$(LOG_SUCCESS)
