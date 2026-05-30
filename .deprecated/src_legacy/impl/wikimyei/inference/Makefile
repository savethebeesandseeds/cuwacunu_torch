# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/wikimyei/inference
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))
# -----------------------------------------------------------------
# Aggregate Targets
# -----------------------------------------------------------------
.PHONY: mdn
mdn:
	$(MAKE) -C $(HERE_PATH)/mdn all

.PHONY: expected_value
expected_value:
	$(MAKE) -C $(HERE_PATH)/expected_value all

.PHONY: all
all: mdn expected_value
	@$(LOG_SUCCESS)
