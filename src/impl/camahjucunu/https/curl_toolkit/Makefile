# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/camahjucunu/https/curl_toolkit
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))
# -----------------------------------------------------------------
# Build Rules (producer; no lib*.a prerequisites)
#   - Use BUILD_OBJ_INC to add CURL headers
#   - Produces: build/libcurl/curl_utils.o
#   - Exposes:  curl_utils as a named goal
# -----------------------------------------------------------------
$(eval $(call BUILD_OBJ_INC, libcurl, curl_utils, curl_utils, $(LIBCURL_INCLUDE_PATHS)))

# -----------------------------------------------------------------
# Aggregate Target
#   - Build local object, then recurse into websockets_api
# -----------------------------------------------------------------
.PHONY: all
all: curl_utils
	$(MAKE) -C $(HERE_PATH)/websockets_api all
	@$(LOG_SUCCESS)
