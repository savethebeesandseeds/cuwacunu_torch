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
#       • https
#       • BNF
#       • exchange
#       • data
# -----------------------------------------------------------------
.PHONY: all
all:
	$(MAKE) -C $(HERE_PATH)/https all
	$(MAKE) -C $(HERE_PATH)/BNF all
	$(MAKE) -C $(HERE_PATH)/exchange all
	$(MAKE) -C $(HERE_PATH)/data all
	@$(LOG_SUCCESS)
