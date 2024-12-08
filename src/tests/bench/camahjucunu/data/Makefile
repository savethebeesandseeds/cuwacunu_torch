# -----------------------------------------------------------------
# Dependencies - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config
# -----------------------------------------------------------------
# Local Makefile Variables - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
HERE_PATH := $(ROOT_PATH)/tests/bench/camahjucunu/data
TEST_BUILD := $(ROOT_PATH)/tests/build
COMMON_TORCH_COMPILE_CMD = \
	$(LOG_START) && $(LOCAL_CLEAN) && \
	$(GXX) $(GXX_FLAGS) \
	$(TORCH_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS) $(USUAL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS) \
	$(OUTPUT_PATH)/common/*.o $(OUTPUT_PATH)/libcurl/*.o $(OUTPUT_PATH)/libtorch/*.o $(OUTPUT_PATH)/openssl/*.o \
	$(SSL_LD_FLAGS) $(LIBCURl_LD_LAGS) $(TORCH_LD_FLAGS) $(USUAL_LD_FLAGS) $< -o $@ && $(LOG_END)
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_memory_mapped_datafile test_memory_mapped_datafile
$(TEST_BUILD)/test_memory_mapped_datafile: $(HERE_PATH)/test_memory_mapped_datafile.cpp
	@$(COMMON_TORCH_COMPILE_CMD)
	$(RUN_MODE) $(TEST_BUILD)/test_memory_mapped_datafile
test_memory_mapped_datafile: $(TEST_BUILD)/test_memory_mapped_datafile
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_memory_mapped_dataset test_memory_mapped_dataset
$(TEST_BUILD)/test_memory_mapped_dataset: $(HERE_PATH)/test_memory_mapped_dataset.cpp
	@$(COMMON_TORCH_COMPILE_CMD)
	$(RUN_MODE) $(TEST_BUILD)/test_memory_mapped_dataset
test_memory_mapped_dataset: $(TEST_BUILD)/test_memory_mapped_dataset
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_memory_mapped_concat_dataset test_memory_mapped_concat_dataset
$(TEST_BUILD)/test_memory_mapped_concat_dataset: $(HERE_PATH)/test_memory_mapped_concat_dataset.cpp
	@$(COMMON_TORCH_COMPILE_CMD)
	$(RUN_MODE) $(TEST_BUILD)/test_memory_mapped_concat_dataset
test_memory_mapped_concat_dataset: $(TEST_BUILD)/test_memory_mapped_concat_dataset
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_dataloader test_dataloader
$(TEST_BUILD)/test_dataloader: $(HERE_PATH)/test_dataloader.cpp
	@$(COMMON_TORCH_COMPILE_CMD)
	$(RUN_MODE) $(TEST_BUILD)/test_dataloader
test_dataloader: $(TEST_BUILD)/test_dataloader
# -----------------------------------------------------------------
# - - - General Target -- - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: all
all: \
	$(TEST_BUILD)/test_memory_mapped_datafile \
	$(TEST_BUILD)/test_memory_mapped_dataset \
	$(TEST_BUILD)/test_dataloader 
	@$(LOG_SUCCESS)
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------