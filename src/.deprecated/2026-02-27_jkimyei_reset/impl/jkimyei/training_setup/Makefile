# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/jkimyei/training_setup
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))
# -----------------------------------------------------------------
# Build Rules
#   - BUILD_OBJ(module, source_basename, phony_target)
#   - Produces: build/<module>/<source_basename>.o
#   - Exposes:  <phony_target> as a named goal
# -----------------------------------------------------------------
$(eval $(call BUILD_OBJ_INC, libtorch, jk_setup, training_setup,   $(TORCH_INCLUDE_PATHS)))

# -----------------------------------------------------------------
# Aggregate Target
#   - "make all" for this directory builds all local objects
# -----------------------------------------------------------------
.PHONY: all
all: training_setup
	@$(LOG_SUCCESS)
