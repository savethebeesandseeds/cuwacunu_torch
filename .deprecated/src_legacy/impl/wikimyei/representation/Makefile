# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/wikimyei/representation
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))
# -----------------------------------------------------------------
# Aggregate Targets
#   - This directory doesn’t build objects itself.
#   - It exposes each submodule as a prerequisite so GNU make can schedule it.
# -----------------------------------------------------------------
.PHONY: VICReg
VICReg:
	$(MAKE) -C $(HERE_PATH)/VICReg all

.PHONY: all
all: VICReg
	@$(LOG_SUCCESS)
