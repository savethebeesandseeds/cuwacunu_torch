# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/wikimyei/inference/expected_value
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))
# -----------------------------------------------------------------
# Aggregate Target
# -----------------------------------------------------------------
.PHONY: mdn
mdn:
	$(MAKE) -C $(HERE_PATH)/mdn all

.PHONY: all
all: mdn
	@$(LOG_SUCCESS)
