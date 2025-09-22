# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/wikimyei/heuristics
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))
# -----------------------------------------------------------------
# Aggregate Target
#   - This directory doesn’t build objects itself.
#   - It recurses into submodules:
#       - representation_learning
#       - semantic_learning
# -----------------------------------------------------------------
.PHONY: all
all:
	$(MAKE) -C $(HERE_PATH)/representation_learning all
	$(MAKE) -C $(HERE_PATH)/semantic_learning all
	@$(LOG_SUCCESS)
