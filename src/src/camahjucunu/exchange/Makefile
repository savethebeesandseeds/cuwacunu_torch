# -----------------------------------------------------------------
# Dependencies - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config
# -----------------------------------------------------------------
# Local Makefile Variables - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
HERE_PATH := $(SRC_PATH)/camahjucunu/exchange
USUAL_DEPS := $(OUTPUT_PATH)/dutils.o $(OUTPUT_PATH)/dconfig.o 
COMPILE_CMD = $(LOG_START) && $(LOCAL_CLEAN) && $(GXX) $(GXX_FLAGS) $(USUAL_INCLUDE_PATHS) -c $< -o $@ && $(LOG_END)
# # -----------------------------------------------------------------
# # - - - Cross files Uitilies  - - - - - - - - - - - - - - - - - - -
# # -----------------------------------------------------------------
EXCHANGE := \
	$(OUTPUT_PATH)/exchange_utils.o \
	$(OUTPUT_PATH)/exchange_types_server.o \
	$(OUTPUT_PATH)/exchange_types_data.o \
	$(OUTPUT_PATH)/exchange_types_trade.o \
	$(OUTPUT_PATH)/exchange_types_account.o
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
$(OUTPUT_PATH)/exchange_utils.o: $(HERE_PATH)/exchange_utils.cpp $(USUAL_DEPS)
	@$(COMPILE_CMD)
$(OUTPUT_PATH)/exchange_types_server.o: $(HERE_PATH)/exchange_types_server.cpp $(USUAL_DEPS) $(OUTPUT_PATH)/exchange_utils.o
	@$(COMPILE_CMD)
$(OUTPUT_PATH)/exchange_types_data.o: $(HERE_PATH)/exchange_types_data.cpp $(USUAL_DEPS) $(OUTPUT_PATH)/exchange_utils.o
	@$(COMPILE_CMD)
$(OUTPUT_PATH)/exchange_types_trade.o: $(HERE_PATH)/exchange_types_trade.cpp $(USUAL_DEPS) $(OUTPUT_PATH)/exchange_utils.o
	@$(COMPILE_CMD)
$(OUTPUT_PATH)/exchange_types_account.o: $(HERE_PATH)/exchange_types_account.cpp $(USUAL_DEPS) $(OUTPUT_PATH)/exchange_utils.o $(OUTPUT_PATH)/exchange_types_trade.o
	@$(COMPILE_CMD)
.PHONY: exchange
exchange: $(EXCHANGE)
	@$(LOG_SUCCESS)
# -----------------------------------------------------------------
# - - - General Target -- - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: all
all: exchange
	$(MAKE) -C $(HERE_PATH)/binance all
	@$(LOG_SUCCESS)
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------