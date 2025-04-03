# -----------------------------------------------------------------
# Dependencies - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config
# -----------------------------------------------------------------
# Local Makefile Variables - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
HERE_PATH := $(SRC_PATH)/camahjucunu/data
USUAL_DEPS := $(OUTPUT_PATH)/common/dutils.o $(OUTPUT_PATH)/common/dconfig.o $(OUTPUT_PATH)/libtorch/torch_utils.o 
COMPILE_WITH_TORCH_CMD = $(LOG_START) && $(LOCAL_CLEAN) && $(GXX) $(GXX_FLAGS) $(USUAL_INCLUDE_PATHS) $(TORCH_INCLUDE_PATHS) -c $< -o $@ && $(LOG_END)
# # -----------------------------------------------------------------
# # - - - Cross files Uitilies  - - - - - - - - - - - - - - - - - - -
# # -----------------------------------------------------------------
DATALOADER_DATASETS := \
	$(OUTPUT_PATH)/libtorch/memory_mapped_datafile.o \
	$(OUTPUT_PATH)/libtorch/memory_mapped_dataset.o \
	$(OUTPUT_PATH)/libtorch/memory_mapped_dataloader.o
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
$(OUTPUT_PATH)/libtorch/memory_mapped_datafile.o: $(HERE_PATH)/memory_mapped_datafile.cpp $(USUAL_DEPS)
	@$(COMPILE_WITH_TORCH_CMD)
$(OUTPUT_PATH)/libtorch/memory_mapped_dataset.o: $(HERE_PATH)/memory_mapped_dataset.cpp $(USUAL_DEPS) $(OUTPUT_PATH)/libtorch/memory_mapped_datafile.o
	@$(COMPILE_WITH_TORCH_CMD)
$(OUTPUT_PATH)/libtorch/memory_mapped_dataloader.o: $(HERE_PATH)/memory_mapped_dataloader.cpp $(USUAL_DEPS) $(OUTPUT_PATH)/libtorch/memory_mapped_dataset.o
	@$(COMPILE_WITH_TORCH_CMD)
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
.PHONY: datasets
datasets: $(DATALOADER_DATASETS)
	@$(LOG_SUCCESS)
# -----------------------------------------------------------------
# - - - General Target -- - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: all
all: datasets
	@$(LOG_SUCCESS)
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------