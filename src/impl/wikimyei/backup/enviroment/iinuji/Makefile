# -----------------------------------------------------------------
# Dependencies - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
ROOT_PATH := ../..
include $(ROOT_PATH)/Makefile.config
# -----------------------------------------------------------------
# Local Makefile Variables - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
HERE_PATH := $(IMPL_PATH)/iinuji/
COMPILE_WITH_TORCH_CMD = $(LOG_START) && $(LOCAL_CLEAN) && $(GXX) $(GXX_FLAGS) $(TORCH_LD_FLAGS) $(USUAL_INCLUDE_PATHS) $(TORCH_INCLUDE_PATHS) -c $< -o $@ && $(LOG_END)
COMPILE_CMD = $(LOG_START) && $(LOCAL_CLEAN) && $(GXX) $(GXX_FLAGS) $(USUAL_INCLUDE_PATHS) -c $< -o $@ && $(LOG_END)
# -----------------------------------------------------------------
# - - - Pytorch's Probability Distributions implemented in Libtorch
# -----------------------------------------------------------------
USUALS_DEPS := $(OUTPUT_PATH)/libtorch/torch_utils.o $(OUTPUT_PATH)/common/dutils.o
IINUJI := $(OUTPUT_PATH)/instrument_space.o \
	$(OUTPUT_PATH)/action_space.o 
# 	$(OUTPUT_PATH)/currency_space.o 
# 	$(OUTPUT_PATH)/order_space.o 
# 	$(OUTPUT_PATH)/position_space.o 
# 	$(OUTPUT_PATH)/quote_space.o 
# 	$(OUTPUT_PATH)/state_space.o 
# 	$(OUTPUT_PATH)/wallet_space.o
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
$(OUTPUT_PATH)/action_space.o: $(HERE_PATH)/action_space.cpp $(USUALS_DEPS)
	@$(COMPILE_WITH_TORCH_CMD)
$(OUTPUT_PATH)/currency_space.o: $(HERE_PATH)/currency_space.cpp $(OUTPUT_PATH)/action_space.o $(USUALS_DEPS)
	@$(COMPILE_WITH_TORCH_CMD)
$(OUTPUT_PATH)/instrument_space.o: $(HERE_PATH)/instrument_space.cpp $(USUALS_DEPS)
	@$(COMPILE_CMD)
$(OUTPUT_PATH)/order_space.o: $(HERE_PATH)/order_space.cpp $(USUALS_DEPS)
	@$(COMPILE_CMD)
$(OUTPUT_PATH)/position_space.o: $(HERE_PATH)/position_space.cpp $(USUALS_DEPS)
	@$(COMPILE_CMD)
$(OUTPUT_PATH)/quote_space.o: $(HERE_PATH)/quote_space.cpp $(USUALS_DEPS)
	@$(COMPILE_CMD)
$(OUTPUT_PATH)/state_space.o: $(HERE_PATH)/state_space.cpp $(USUALS_DEPS)
	@$(COMPILE_CMD)
$(OUTPUT_PATH)/wallet_space.o: $(HERE_PATH)/wallet_space.cpp $(USUALS_DEPS)
	@$(COMPILE_CMD)
.PHONY: iinuji
iinuji: $(IINUJI)
	@$(LOG_SUCCESS)
# -----------------------------------------------------------------
# - - - General Target -- - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: all
all: iinuji
	@$(LOG_SUCCESS)