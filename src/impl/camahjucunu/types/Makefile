# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/camahjucunu/types
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))
# -----------------------------------------------------------------
# Build Rules (producers; no lib*.a prerequisites)
#   - Use BUILD_OBJ_INC to add CURL/SSL headers
#   - Produces: build/libcurl/*.o
# -----------------------------------------------------------------
$(eval $(call BUILD_OBJ_INC, libcurl, types_utils,    types_utils,    $(LIBCURL_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS)))
$(eval $(call BUILD_OBJ_INC, libcurl, types_server,   types_server,   $(LIBCURL_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS)))
$(eval $(call BUILD_OBJ_INC, libcurl, types_data,     types_data,     $(LIBCURL_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS)))
$(eval $(call BUILD_OBJ_INC, libcurl, types_trade,    types_trade,    $(LIBCURL_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS)))
$(eval $(call BUILD_OBJ_INC, libcurl, types_account,  types_account,  $(LIBCURL_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS)))

# Grouped objects for a convenient target
EXCHANGE := \
  $(OUTPUT_PATH)/libcurl/types_utils.o \
  $(OUTPUT_PATH)/libcurl/types_server.o \
  $(OUTPUT_PATH)/libcurl/types_data.o \
  $(OUTPUT_PATH)/libcurl/types_trade.o \
  $(OUTPUT_PATH)/libcurl/types_account.o

# -----------------------------------------------------------------
# Aggregate Targets
#   - Build local objects
# -----------------------------------------------------------------
.PHONY: exchange
exchange: $(EXCHANGE)
	@$(LOG_SUCCESS)

.PHONY: all
all: exchange
	@$(LOG_SUCCESS)
