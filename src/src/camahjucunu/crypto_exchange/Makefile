# -----------------------------------------------------------------
# Dependencies - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config
# -----------------------------------------------------------------
# Local Makefile Variables - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
HERE_PATH := $(SRC_PATH)/camahjucunu/crypto_exchange
COMPILE_CMD = $(LOG_START) && $(LOCAL_CLEAN) && $(GXX) $(GXX_FLAGS) $(USUAL_INCLUDE_PATHS) -c $< -o $@ && $(LOG_END)
# # -----------------------------------------------------------------
# # - - - Cross files Uitilies  - - - - - - - - - - - - - - - - - - -
# # -----------------------------------------------------------------
CRYPTO_EXCHANGE := $(OUTPUT_PATH)/binance_enums.o $(OUTPUT_PATH)/binance_types.o $(OUTPUT_PATH)/binance_serialization.o $(OUTPUT_PATH)/binance_deserialization.o
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
$(OUTPUT_PATH)/binance_enums.o: $(HERE_PATH)/binance_enums.cpp $(OUTPUT_PATH)/dutils.o
	@$(COMPILE_CMD)
$(OUTPUT_PATH)/binance_serialization.o: $(HERE_PATH)/binance_serialization.cpp $(OUTPUT_PATH)/dutils.o
	@$(COMPILE_CMD)
$(OUTPUT_PATH)/binance_deserialization.o: $(HERE_PATH)/binance_deserialization.cpp $(OUTPUT_PATH)/dutils.o
	@$(COMPILE_CMD)
$(OUTPUT_PATH)/binance_types.o: $(HERE_PATH)/binance_types.cpp $(OUTPUT_PATH)/dutils.o $(OUTPUT_PATH)/binance_enums.o $(OUTPUT_PATH)/binance_serialization.o $(OUTPUT_PATH)/binance_deserialization.o
	@$(COMPILE_CMD)
.PHONY: crypto_exchange
crypto_exchange: $(CRYPTO_EXCHANGE)
	@$(LOG_SUCCESS)
# -----------------------------------------------------------------
# - - - General Target -- - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: all
all: $(CRYPTO_EXCHANGE)
	@$(LOG_SUCCESS)
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------