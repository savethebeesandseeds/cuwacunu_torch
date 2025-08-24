# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/camahjucunu/exchange/binance
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))
# -----------------------------------------------------------------
# Build Rules (producers; no lib*.a prerequisites)
#   - Use BUILD_OBJ_INC to add CURL/SSL headers for these objects
#   - Produces: build/libcurl/*.o
# -----------------------------------------------------------------
$(eval $(call BUILD_OBJ_INC, libcurl, binance_utils,         binance_utils,         $(LIBCURL_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS)))
$(eval $(call BUILD_OBJ_INC, libcurl, binance_mech_server,   binance_mech_server,   $(LIBCURL_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS)))
$(eval $(call BUILD_OBJ_INC, libcurl, binance_mech_data,     binance_mech_data,     $(LIBCURL_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS)))
$(eval $(call BUILD_OBJ_INC, libcurl, binance_mech_trade,    binance_mech_trade,    $(LIBCURL_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS)))
$(eval $(call BUILD_OBJ_INC, libcurl, binance_mech_account,  binance_mech_account,  $(LIBCURL_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS)))

# Grouped objects for convenience targets
BINANCE_MECH := \
  $(OUTPUT_PATH)/libcurl/binance_utils.o \
  $(OUTPUT_PATH)/libcurl/binance_mech_server.o \
  $(OUTPUT_PATH)/libcurl/binance_mech_data.o \
  $(OUTPUT_PATH)/libcurl/binance_mech_trade.o \
  $(OUTPUT_PATH)/libcurl/binance_mech_account.o

# -----------------------------------------------------------------
# Aggregate Targets
# -----------------------------------------------------------------
.PHONY: binance_mech
binance_mech: $(BINANCE_MECH)
	@$(LOG_SUCCESS)

.PHONY: all
all: binance_mech
	@$(LOG_SUCCESS)
