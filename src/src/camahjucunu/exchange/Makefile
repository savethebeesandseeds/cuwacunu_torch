# -----------------------------------------------------------------
# Dependencies - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config
# -----------------------------------------------------------------
# Local Makefile Variables - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
HERE_PATH := $(SRC_PATH)/camahjucunu/exchange
COMPILE_CMD = $(LOG_START) && $(LOCAL_CLEAN) && $(GXX) $(GXX_FLAGS) $(USUAL_INCLUDE_PATHS) -c $< -o $@ && $(LOG_END)
# # -----------------------------------------------------------------
# # - - - Cross files Uitilies  - - - - - - - - - - - - - - - - - - -
# # -----------------------------------------------------------------
EXCHANGE := $(OUTPUT_PATH)/exchange_enums.o $(OUTPUT_PATH)/exchange_types.o $(OUTPUT_PATH)/exchange_serialization.o $(OUTPUT_PATH)/exchange_deserialization.o
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
$(OUTPUT_PATH)/exchange_enums.o: $(HERE_PATH)/exchange_enums.cpp $(OUTPUT_PATH)/dutils.o
	@$(COMPILE_CMD)
$(OUTPUT_PATH)/exchange_serialization.o: $(HERE_PATH)/exchange_serialization.cpp $(OUTPUT_PATH)/dutils.o
	@$(COMPILE_CMD)
$(OUTPUT_PATH)/exchange_deserialization.o: $(HERE_PATH)/exchange_deserialization.cpp $(OUTPUT_PATH)/dutils.o
	@$(COMPILE_CMD)
$(OUTPUT_PATH)/exchange_types.o: $(HERE_PATH)/exchange_types.cpp $(OUTPUT_PATH)/dutils.o $(OUTPUT_PATH)/exchange_enums.o $(OUTPUT_PATH)/exchange_serialization.o $(OUTPUT_PATH)/exchange_deserialization.o
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