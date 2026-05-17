# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/wikimyei
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))
# -----------------------------------------------------------------
# Aggregate Targets
#   - This directory doesn’t build objects itself.
#   - It fans out into submodules so GNU make can schedule them in parallel.
# -----------------------------------------------------------------
.PHONY: representation
representation:
	$(MAKE) -C $(HERE_PATH)/representation all

.PHONY: inference
inference:
	$(MAKE) -C $(HERE_PATH)/inference all

.PHONY: all
all: representation inference
	@$(LOG_SUCCESS)
