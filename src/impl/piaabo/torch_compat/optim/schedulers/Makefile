# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../../../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/piaabo/torch_compat/optim/schedulers
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))
# -----------------------------------------------------------------
# Build Rules (producer; no lib*.a prerequisites)
#   - Use BUILD_OBJ_INC to add Torch headers
#   - Produces: build/libtorch/lambda_lr_scheduler.o
#   - Exposes:  schedulers as a named goal
# -----------------------------------------------------------------
$(eval $(call BUILD_OBJ_INC, libtorch, lambda_lr_scheduler, schedulers, $(TORCH_INCLUDE_PATHS)))

# -----------------------------------------------------------------
# Aggregate Target
# -----------------------------------------------------------------
.PHONY: all
all: schedulers
	@$(LOG_SUCCESS)
