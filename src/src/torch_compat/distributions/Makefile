# -----------------------------------------------------------------
# Dependencies - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config
# -----------------------------------------------------------------
# Local Makefile Variables - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
HERE_PATH := $(SRC_PATH)/torch_compat/distributions
COMPILE_CMD = $(LOG_START) && $(GXX) $(GXX_FLAGS) $(USUAL_INCLUDE_PATHS) $(TORCH_INCLUDE_PATHS) -c $< -o $@ && $(LOG_END)
# -----------------------------------------------------------------
# - - - Pytorch's Probability Distributions implemented in Libtorch
# -----------------------------------------------------------------
USUALS_DEPS := $(OUTPUT_PATH)/torch_utils.o $(OUTPUT_PATH)/dutils.o
DISTRIBUTIONS := $(OUTPUT_PATH)/gamma.o $(OUTPUT_PATH)/beta.o $(OUTPUT_PATH)/categorical.o
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
$(OUTPUT_PATH)/gamma.o: $(HERE_PATH)/gamma.cpp $(USUALS_DEPS)
	@$(COMPILE_CMD)
$(OUTPUT_PATH)/beta.o: $(HERE_PATH)/beta.cpp $(OUTPUT_PATH)/gamma.o $(USUALS_DEPS)
	@$(COMPILE_CMD)
$(OUTPUT_PATH)/categorical.o: $(HERE_PATH)/categorical.cpp $(USUALS_DEPS)
	@$(COMPILE_CMD)
.PHONY: distributions
distributions: $(DISTRIBUTIONS)
	@$(LOG_SUCCESS)
# -----------------------------------------------------------------
# - - - General Target -- - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: all
all: distributions
	@$(LOG_SUCCESS)
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------