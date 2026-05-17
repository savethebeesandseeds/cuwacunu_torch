# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/piaabo/https_compat
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))
# -----------------------------------------------------------------
# Aggregate Targets
#   - This directory doesn’t build objects itself.
#   - It fans out into curl_toolkit so the parent graph can schedule it as a
#     prerequisite instead of serializing it in a recipe.
# -----------------------------------------------------------------
.PHONY: curl_toolkit
curl_toolkit:
	$(MAKE) -C $(HERE_PATH)/curl_toolkit all

.PHONY: all
all: curl_toolkit
	@$(LOG_SUCCESS)
