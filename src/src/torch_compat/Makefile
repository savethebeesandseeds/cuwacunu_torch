# -----------------------------------------------------------------
# Dependencies - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
ROOT_PATH := ../..
include $(ROOT_PATH)/Makefile.config
# -----------------------------------------------------------------
# Local Makefile Variables - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
HERE_PATH := $(SRC_PATH)/torch_compat
COMPILE_CMD = $(LOG_START) && $(GXX) $(GXX_FLAGS) $(USUAL_INCLUDE_PATHS) $(TORCH_INCLUDE_PATHS) -c $< -o $@ && $(LOG_END)
# -----------------------------------------------------------------
# - - - Libtorch Utilities  - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
TORCH_UTILS := $(OUTPUT_PATH)/torch_utils.o 
$(TORCH_UTILS): $(HERE_PATH)/torch_utils.cpp $(OUTPUT_PATH)/dutils.o
	@$(COMPILE_CMD)
.PHONY: torch_utils
torch_utils: $(TORCH_UTILS)
	@$(LOG_SUCCESS)
# -----------------------------------------------------------------
# - - - General Target -- - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: all
all: torch_utils
	$(MAKE) -C $(HERE_PATH)/distributions all
	$(MAKE) -C $(HERE_PATH)/optim/schedulers all
	@$(LOG_SUCCESS)
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------