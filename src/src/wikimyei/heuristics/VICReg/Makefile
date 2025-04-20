# -----------------------------------------------------------------
# Dependencies - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config
# -----------------------------------------------------------------
# Local Makefile Variables - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
HERE_PATH := $(SRC_PATH)/wikimyei/heuristics/VICReg
COMMON_TORCH_COMPILE_CMD = \
	$(LOG_START) && $(LOCAL_CLEAN) && \
	$(GXX) $(GXX_FLAGS) \
	$(TORCH_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS) $(USUAL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS) \
	$(SSL_LD_FLAGS) $(LIBCURL_LD_LAGS) $(TORCH_LD_FLAGS) $(USUAL_LD_FLAGS) -c $< -o $@ && $(LOG_END)
# -----------------------------------------------------------------
# - - - - - - - - - - - vicreg_4d  - - - - - - - - - - - - - - - - - -  
# -----------------------------------------------------------------
TS2VEC_TARGET := $(OUTPUT_PATH)/libtorch/vicreg_4d.o

$(TS2VEC_TARGET): $(HERE_PATH)/vicreg_4d.cpp $(OUTPUT_PATH)/common/dutils.o $(OUTPUT_PATH)/libtorch/torch_utils.o
	@$(COMMON_TORCH_COMPILE_CMD)
.PHONY: vicreg_4d
vicreg_4d: $(TS2VEC_TARGET)
	@$(LOG_SUCCESS)
# -----------------------------------------------------------------
# - - - General Target -- - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: all
all: vicreg_4d
	@$(LOG_SUCCESS)
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------