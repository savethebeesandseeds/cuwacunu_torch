# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../../../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/piaabo/https_compat/curl_toolkit/websockets_api
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))
# -----------------------------------------------------------------
# Build Rules (producer; no lib*.a prerequisites)
#   - Use BUILD_OBJ_INC to include CURL/SSL headers
#   - Produces: build/libcurl/curl_websocket_api.o
#   - Exposes:  websocket_api as a named goal
# -----------------------------------------------------------------
$(eval $(call BUILD_OBJ_INC, libcurl, curl_websocket_api, websocket_api, $(LIBCURL_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS)))

# -----------------------------------------------------------------
# Aggregate Target
# -----------------------------------------------------------------
.PHONY: all
all: websocket_api
	@$(LOG_SUCCESS)
