# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/jkimyei
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))
# -----------------------------------------------------------------
# Aggregate Target
#   - This directory doesn’t build objects itself.
#   - It recurses into submodules:
#       - training_setup
# -----------------------------------------------------------------
.PHONY: all
all:
	$(MAKE) -C $(HERE_PATH)/training_setup all
	@$(LOG_SUCCESS)
