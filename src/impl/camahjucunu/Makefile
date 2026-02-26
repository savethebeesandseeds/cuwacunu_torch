# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/camahjucunu
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))
# -----------------------------------------------------------------
# Aggregate Target
#   - This directory doesn’t build objects itself.
#   - It recurses into submodules:
#       - dsl
#       - exchange
#       - data
# -----------------------------------------------------------------
.PHONY: all
all:
	$(MAKE) -C $(HERE_PATH)/dsl all
	$(MAKE) -C $(HERE_PATH)/types all
	$(MAKE) -C $(HERE_PATH)/db all
	$(MAKE) -C $(HERE_PATH)/exchange all
	$(MAKE) -C $(HERE_PATH)/data all
	@$(LOG_SUCCESS)
