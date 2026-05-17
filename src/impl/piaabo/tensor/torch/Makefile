# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/piaabo/tensor/torch
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))
# -----------------------------------------------------------------
# Build Rules (producer; no lib*.a prerequisites)
#   - Use BUILD_OBJ_INC to add Torch headers
#   - Produces: build/libtorch/torch_utils.o, config_adapter.o
#   - Exposes:  torch_utils/config_adapter as named goals
# -----------------------------------------------------------------
$(eval $(call BUILD_OBJ_INC, libtorch, torch_utils, torch_utils, $(TORCH_INCLUDE_PATHS)))
$(eval $(call BUILD_OBJ_INC, libtorch, config_adapter, config_adapter, $(TORCH_INCLUDE_PATHS)))

# -----------------------------------------------------------------
# Aggregate Targets
#   - Expose distributions as a sibling prerequisite so GNU make can schedule
#     it in parallel with the local tensor/torch producers.
# -----------------------------------------------------------------
.PHONY: distributions
distributions:
	$(MAKE) -C $(HERE_PATH)/distributions all

.PHONY: all
all: torch_utils config_adapter distributions
	@$(LOG_SUCCESS)
