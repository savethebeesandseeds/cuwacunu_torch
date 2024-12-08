# -----------------------------------------------------------------
# Dependencies - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
ROOT_PATH := ../../../../..
include $(ROOT_PATH)/Makefile.config
# -----------------------------------------------------------------
# Local Makefile Variables - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
HERE_PATH := $(SRC_PATH)/camahjucunu/https/curl_toolkit/websockets_api
USUAL_DEPS := $(OUTPUT_PATH)/common/dutils.o $(OUTPUT_PATH)/common/dconfig.o $(OUTPUT_PATH)/common/darchitecture.o $(OUTPUT_PATH)/common/djson_parsing.o 
COMPILE_CMD = $(LOG_START) && $(LOCAL_CLEAN) && $(GXX) $(GXX_FLAGS) $(USUAL_INCLUDE_PATHS) -c $< -o $@ && $(LOG_END)
# # -----------------------------------------------------------------
# # - - - Cross files Uitilies  - - - - - - - - - - - - - - - - - - -
# # -----------------------------------------------------------------
WEB_SOCKET := \
	$(OUTPUT_PATH)/libcurl/curl_websocket_api.o
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
$(OUTPUT_PATH)/libcurl/curl_websocket_api.o: $(HERE_PATH)/curl_websocket_api.cpp $(USUAL_DEPS)
	@$(COMPILE_CMD)
.PHONY: WEB_SOCKET
WEB_SOCKET: $(WEB_SOCKET)
	@$(LOG_SUCCESS)
# -----------------------------------------------------------------
# - - - General Target -- - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: all
all: WEB_SOCKET
	@$(LOG_SUCCESS)
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------