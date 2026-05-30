# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/wikimyei/representation/VICReg
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))
# -----------------------------------------------------------------
# Build Rules (producer; no lib*.a prerequisites)
#   - Use BUILD_OBJ_INC to add Torch (+SSL/CURL) headers
#   - Produces: build/libtorch/vicreg_rank4.o
#   - Exposes:  vicreg_rank4 as a named goal
# -----------------------------------------------------------------
$(eval $(call BUILD_OBJ_INC, libtorch, vicreg_rank4, vicreg_rank4, \
  $(TORCH_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS)))

# -----------------------------------------------------------------
# Aggregate Target
# -----------------------------------------------------------------
.PHONY: all
all: vicreg_rank4
	@$(LOG_SUCCESS)
