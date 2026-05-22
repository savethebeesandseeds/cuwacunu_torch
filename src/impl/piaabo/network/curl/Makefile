# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/piaabo/network/curl
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))
# -----------------------------------------------------------------
# Build Rules (producer; no lib*.a prerequisites)
#   - Use BUILD_OBJ_INC to add CURL headers
#   - Produces: build/libcurl/curl_utils.o
#   - Exposes:  curl_utils as a named goal
# -----------------------------------------------------------------
$(eval $(call BUILD_OBJ_INC, libcurl, curl_utils, curl_utils, $(LIBCURL_INCLUDE_PATHS)))
$(eval $(call BUILD_OBJ_INC, libcurl, openai_responses_api, openai_responses_api, $(LIBCURL_INCLUDE_PATHS)))

# -----------------------------------------------------------------
# Aggregate Targets
#   - Expose websocket_api as a sibling prerequisite so GNU make can schedule
#     it in parallel with the local curl producers.
# -----------------------------------------------------------------
.PHONY: websocket_api
websocket_api:
	$(MAKE) -C $(HERE_PATH)/websocket_api all

.PHONY: all
all: curl_utils openai_responses_api websocket_api
	@$(LOG_SUCCESS)
