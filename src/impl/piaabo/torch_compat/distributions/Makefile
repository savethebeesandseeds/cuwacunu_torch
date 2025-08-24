# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/piaabo/torch_compat/distributions
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))
# -----------------------------------------------------------------
# Build Rules (producers; no lib*.a prerequisites)
#   - Use BUILD_OBJ_INC to add Torch headers
#   - Produces: build/libtorch/{gamma.o,beta.o,categorical.o}
#   - Exposes:  distributions as a named goal
# -----------------------------------------------------------------
$(eval $(call BUILD_OBJ_INC, libtorch, gamma,       gamma,       $(TORCH_INCLUDE_PATHS)))
$(eval $(call BUILD_OBJ_INC, libtorch, beta,        beta,        $(TORCH_INCLUDE_PATHS)))
$(eval $(call BUILD_OBJ_INC, libtorch, categorical, categorical, $(TORCH_INCLUDE_PATHS)))

DISTRIBUTIONS := \
  $(OUTPUT_PATH)/libtorch/gamma.o \
  $(OUTPUT_PATH)/libtorch/beta.o \
  $(OUTPUT_PATH)/libtorch/categorical.o

# -----------------------------------------------------------------
# Aggregate Target
# -----------------------------------------------------------------
.PHONY: distributions
distributions: $(DISTRIBUTIONS)
	@$(LOG_SUCCESS)

# -----------------------------------------------------------------
# All
# -----------------------------------------------------------------
.PHONY: all
all: distributions
	@$(LOG_SUCCESS)
