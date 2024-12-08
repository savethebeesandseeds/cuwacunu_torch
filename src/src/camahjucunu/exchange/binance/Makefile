# -----------------------------------------------------------------
# Dependencies - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config
# -----------------------------------------------------------------
# Local Makefile Variables - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
HERE_PATH := $(SRC_PATH)/camahjucunu/exchange/binance
USUAL_DEPS := \
	$(OUTPUT_PATH)/common/dutils.o $(OUTPUT_PATH)/common/dconfig.o $(OUTPUT_PATH)/libcurl/exchange_utils.o \
	$(OUTPUT_PATH)/libcurl/exchange_utils.o \
	$(OUTPUT_PATH)/libcurl/exchange_types_server.o \
	$(OUTPUT_PATH)/libcurl/exchange_types_data.o \
	$(OUTPUT_PATH)/libcurl/exchange_types_trade.o \
	$(OUTPUT_PATH)/libcurl/exchange_types_account.o
COMPILE_CMD = $(LOG_START) && $(LOCAL_CLEAN) && $(GXX) $(GXX_FLAGS) $(USUAL_INCLUDE_PATHS) -c $< -o $@ && $(LOG_END)
# # -----------------------------------------------------------------
# # - - - Cross files Uitilies  - - - - - - - - - - - - - - - - - - -
# # -----------------------------------------------------------------
BINANCE_MECH := \
	$(OUTPUT_PATH)/libcurl/binance_utils.o \
	$(OUTPUT_PATH)/libcurl/binance_mech_server.o \
	$(OUTPUT_PATH)/libcurl/binance_mech_data.o \
	$(OUTPUT_PATH)/libcurl/binance_mech_trade.o \
	$(OUTPUT_PATH)/libcurl/binance_mech_account.o
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
$(OUTPUT_PATH)/libcurl/binance_utils.o: $(HERE_PATH)/binance_utils.cpp $(USUAL_DEPS)
	@$(COMPILE_CMD)
$(OUTPUT_PATH)/libcurl/binance_mech_server.o: $(HERE_PATH)/binance_mech_server.cpp $(USUAL_DEPS) $(OUTPUT_PATH)/libcurl/binance_utils.o
	@$(COMPILE_CMD)
$(OUTPUT_PATH)/libcurl/binance_mech_data.o: $(HERE_PATH)/binance_mech_data.cpp $(USUAL_DEPS) $(OUTPUT_PATH)/libcurl/binance_utils.o
	@$(COMPILE_CMD)
$(OUTPUT_PATH)/libcurl/binance_mech_trade.o: $(HERE_PATH)/binance_mech_trade.cpp $(USUAL_DEPS) $(OUTPUT_PATH)/libcurl/binance_utils.o
	@$(COMPILE_CMD)
$(OUTPUT_PATH)/libcurl/binance_mech_account.o: $(HERE_PATH)/binance_mech_account.cpp $(USUAL_DEPS) $(OUTPUT_PATH)/libcurl/binance_utils.o
	@$(COMPILE_CMD)
.PHONY: binance_mech_account
binance_mech_account: $(BINANCE_MECH)
	@$(LOG_SUCCESS)
# -----------------------------------------------------------------
# - - - General Target -- - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: all
all: $(BINANCE_MECH)
	@$(LOG_SUCCESS)
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------