# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../../../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/camahjucunu/BNF/implementations/training_components
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))
# -----------------------------------------------------------------
# Build Rules
#   - BUILD_OBJ(module, source_basename, phony_target)
#   - Produces: build/<module>/<source_basename>.o
#   - Exposes:  <phony_target> as a named goal
# -----------------------------------------------------------------
$(eval $(call BUILD_OBJ, common, training_components, bnf_train_pipe))

# -----------------------------------------------------------------
# Aggregate Target
#   - "make all" for this directory builds all local objects
# -----------------------------------------------------------------
.PHONY: all
all: bnf_train_pipe
	@$(LOG_SUCCESS)
