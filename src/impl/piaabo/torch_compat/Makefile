# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/piaabo/torch_compat
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))
# -----------------------------------------------------------------
# Build Rules (producer; no lib*.a prerequisites)
#   - Use BUILD_OBJ_INC to add Torch headers
#   - Produces: build/libtorch/torch_utils.o
#   - Exposes:  torch_utils as a named goal
# -----------------------------------------------------------------
$(eval $(call BUILD_OBJ_INC, libtorch, torch_utils, torch_utils, $(TORCH_INCLUDE_PATHS)))

# -----------------------------------------------------------------
# Aggregate Target
#   - Build local object, then recurse into submodules:
#       • distributions
#       • optim/schedulers
# -----------------------------------------------------------------
.PHONY: all
all: torch_utils
	$(MAKE) -C $(HERE_PATH)/distributions all
	$(MAKE) -C $(HERE_PATH)/optim/schedulers all
	@$(LOG_SUCCESS)
