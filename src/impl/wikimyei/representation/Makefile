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
.PHONY: encoding_vicreg
encoding_vicreg:
	$(MAKE) -C $(HERE_PATH)/encoding/vicreg all

.PHONY: all
all: encoding_vicreg
	@$(LOG_SUCCESS)
