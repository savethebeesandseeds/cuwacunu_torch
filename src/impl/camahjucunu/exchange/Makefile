# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/camahjucunu/exchange
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))
# -----------------------------------------------------------------
# Build Rules (producers; no lib*.a prerequisites)
#   - Use BUILD_OBJ_INC to add CURL/SSL headers
#   - Produces: build/libcurl/*.o
# -----------------------------------------------------------------
$(eval $(call BUILD_OBJ_INC, libcurl, exchange_utils,          exchange_utils,          $(LIBCURL_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS)))
$(eval $(call BUILD_OBJ_INC, libcurl, exchange_types_server,   exchange_types_server,   $(LIBCURL_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS)))
$(eval $(call BUILD_OBJ_INC, libcurl, exchange_types_data,     exchange_types_data,     $(LIBCURL_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS)))
$(eval $(call BUILD_OBJ_INC, libcurl, exchange_types_trade,    exchange_types_trade,    $(LIBCURL_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS)))
$(eval $(call BUILD_OBJ_INC, libcurl, exchange_types_account,  exchange_types_account,  $(LIBCURL_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS)))

# Grouped objects for a convenient target
EXCHANGE := \
  $(OUTPUT_PATH)/libcurl/exchange_utils.o \
  $(OUTPUT_PATH)/libcurl/exchange_types_server.o \
  $(OUTPUT_PATH)/libcurl/exchange_types_data.o \
  $(OUTPUT_PATH)/libcurl/exchange_types_trade.o \
  $(OUTPUT_PATH)/libcurl/exchange_types_account.o

# -----------------------------------------------------------------
# Aggregate Targets
#   - Build local objects, then recurse into exchange/binance
# -----------------------------------------------------------------
.PHONY: exchange
exchange: $(EXCHANGE)
	@$(LOG_SUCCESS)

.PHONY: all
all: exchange
	$(MAKE) -C $(HERE_PATH)/binance all
	@$(LOG_SUCCESS)
