unfinished. not functional
# -----------------------------------------------------------------
# Dependencies - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config
# -----------------------------------------------------------------
# Local Makefile Variables - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
HERE_PATH := $(SRC_PATH)/wikimyei/heuristics/ts2vec
COMMON_TORCH_COMPILE_CMD = \
	$(LOG_START) && $(LOCAL_CLEAN) && \
	$(GXX) $(GXX_FLAGS) \
	$(TORCH_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS) $(USUAL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS) \
	$(SSL_LD_FLAGS) $(LIBCURL_LD_LAGS) $(TORCH_LD_FLAGS) $(USUAL_LD_FLAGS) -c $< -o $@ && $(LOG_END)
# -----------------------------------------------------------------
# - - - - - - - - - - - ts2vec  - - - - - - - - - - - - - - - - - -  
# -----------------------------------------------------------------
TS2VEC_TARGET := $(OUTPUT_PATH)/libtorch/ts2vec.o

$(TS2VEC_TARGET): $(HERE_PATH)/ts2vec.cpp $(OUTPUT_PATH)/common/dutils.o $(OUTPUT_PATH)/libtorch/torch_utils.o
	@$(COMMON_TORCH_COMPILE_CMD)
.PHONY: ts2vec
ts2vec: $(TS2VEC_TARGET)
	@$(LOG_SUCCESS)
# -----------------------------------------------------------------
# - - - General Target -- - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: all
all: ts2vec
	@$(LOG_SUCCESS)
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------