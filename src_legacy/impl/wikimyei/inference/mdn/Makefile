# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/wikimyei/inference/mdn
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))
# -----------------------------------------------------------------
# Build Rules (producer; no lib*.a prerequisites)
# -----------------------------------------------------------------
$(eval $(call BUILD_OBJ_INC, libtorch, mixture_density_network, mixture_density_network, \
  $(TORCH_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS)))

# -----------------------------------------------------------------
# Aggregate Target
# -----------------------------------------------------------------
.PHONY: all
all: mixture_density_network
	@$(LOG_SUCCESS)
