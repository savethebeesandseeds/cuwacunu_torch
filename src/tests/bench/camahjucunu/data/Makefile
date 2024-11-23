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
# EXTENDED_TORCH_LINK_CMD = $(LOG_START) && $(LOCAL_CLEAN) && $(GXX) $(GXX_FLAGS) $(USUAL_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS) $(TORCH_INCLUDE_PATHS) $(OBJECT_TO_LINK) $(LIBCURl_LD_LAGS) $(SSL_LD_FLAGS) $(USUAL_LD_FLAGS) $(TORCH_LD_FLAGS) $< -o $@ && $(LOG_END)
# EXTENDED_LINK_CMD = $(LOG_START) && $(LOCAL_CLEAN) && $(GXX) $(GXX_FLAGS) $(USUAL_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS) $(OBJECT_TO_LINK) $(LIBCURl_LD_LAGS) $(SSL_LD_FLAGS) $(USUAL_LD_FLAGS) $< -o $@ && $(LOG_END)
# COMPILE_WITH_TORCH_CMD = $(GXX) $(GXX_FLAGS) $(USUAL_INCLUDE_PATHS) $(TORCH_INCLUDE_PATHS) $< -o $@

# COMPILE_WITH_TORCH_CMD = $(LOG_START) && $(LOCAL_CLEAN) && $(GXX) $(GXX_FLAGS) $(USUAL_INCLUDE_PATHS)  $< -o $@ && $(LOG_END)
# COMPILE_WITH_TORCH_CMD = $(LOG_START) && $(LOCAL_CLEAN) && $(GXX) $(GXX_FLAGS) $(USUAL_INCLUDE_PATHS) $(TORCH_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS) $(OBJECT_TO_LINK) $(TORCH_LD_FLAGS) $(LIBCURl_LD_LAGS) $(SSL_LD_FLAGS) $(USUAL_LD_FLAGS) $< -o $@ && $(LOG_END)

COMPILE_WITH_TORCH_CMD = $(LOG_START) && $(LOCAL_CLEAN) && $(GXX) $(GXX_FLAGS) $< $(OBJECT_TO_LINK) $(USUAL_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS) $(TORCH_INCLUDE_PATHS) $(TORCH_LD_FLAGS) $(LIBCURl_LD_LAGS) $(SSL_LD_FLAGS) $(USUAL_LD_FLAGS) -o $@ && $(LOG_END)
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_memory_mapped_dataset test_memory_mapped_dataset
$(TEST_BUILD)/test_memory_mapped_dataset: $(HERE_PATH)/test_memory_mapped_dataset.cpp
	$(RUN_MODE) $(TEST_BUILD)/test_memory_mapped_dataset
test_memory_mapped_dataset: $(TEST_BUILD)/test_memory_mapped_dataset
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_dataloader test_dataloader
$(TEST_BUILD)/test_dataloader: $(HERE_PATH)/test_dataloader.cpp
	$(COMPILE_WITH_TORCH_CMD)
	$(RUN_MODE) $(TEST_BUILD)/test_dataloader
test_dataloader: $(TEST_BUILD)/test_dataloader
# -----------------------------------------------------------------
# - - - General Target -- - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: all
all: \
	$(TEST_BUILD)/test_memory_mapped_dataset \
	$(TEST_BUILD)/test_dataloader 
	@$(LOG_SUCCESS)
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------