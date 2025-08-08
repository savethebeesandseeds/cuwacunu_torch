# -----------------------------------------------------------------
# Dependencies - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
ROOT_PATH := ../../../../..
include $(ROOT_PATH)/Makefile.config
# -----------------------------------------------------------------
# Local Makefile Variables - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
HERE_PATH := $(SRC_PATH)/wikimyei/heuristics/representation_learning/VICReg
COMMON_TORCH_COMPILE_CMD = \
    $(LOG_START) && $(LOCAL_CLEAN) && \
    $(GXX) $(GXX_FLAGS) \
      $(TORCH_INCLUDE_PATHS) \
      $(SSL_INCLUDE_PATHS) \
      $(USUAL_INCLUDE_PATHS) \
      $(LIBCURL_INCLUDE_PATHS) \
      -c $< -o $@ && $(LOG_END)
# -----------------------------------------------------------------
# - - - - - - - - - - - vicreg_4d  - - - - - - - - - - - - - - - - - -  
# -----------------------------------------------------------------
TARGET := $(OUTPUT_PATH)/libtorch/vicreg_4d.o
$(TARGET): $(HERE_PATH)/vicreg_4d.cpp $(OUTPUT_PATH)/common/dutils.o $(OUTPUT_PATH)/libtorch/torch_utils.o
	@$(COMMON_TORCH_COMPILE_CMD)

.PHONY: vicreg_4d
vicreg_4d: $(TARGET)
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