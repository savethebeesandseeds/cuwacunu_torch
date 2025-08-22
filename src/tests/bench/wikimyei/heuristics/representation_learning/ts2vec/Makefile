# -----------------------------------------------------------------
# Dependencies - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
ROOT_PATH := ../../../../..
include $(ROOT_PATH)/Makefile.config
# -----------------------------------------------------------------
# Local Makefile Variables - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
HERE_PATH := $(ROOT_PATH)/tests/bench/wikimyei/heuristics/ts2vec
TEST_BUILD := $(ROOT_PATH)/tests/build
COMMON_TORCH_COMPILE_CMD = \
	$(LOG_START) && $(LOCAL_CLEAN) && \
	$(GXX) $(GXX_FLAGS) \
	$(TORCH_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS) $(USUAL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS) \
	$(OUTPUT_PATH)/common/*.o $(OUTPUT_PATH)/libcurl/*.o $(OUTPUT_PATH)/libtorch/*.o $(OUTPUT_PATH)/openssl/*.o \
	$(SSL_LD_FLAGS) $(LIBCURL_LD_LAGS) $(TORCH_LD_FLAGS) $(USUAL_LD_FLAGS) $< -o $@ && $(LOG_END)
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_ts2vec_datautils test_ts2vec_datautils
$(TEST_BUILD)/test_ts2vec_datautils: $(HERE_PATH)/test_ts2vec_datautils.cpp
	@$(COMMON_TORCH_COMPILE_CMD)
	$(RUN_MODE) $(TEST_BUILD)/test_ts2vec_datautils
test_ts2vec_datautils: $(TEST_BUILD)/test_ts2vec_datautils
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_ts2vec_losses test_ts2vec_losses
$(TEST_BUILD)/test_ts2vec_losses: $(HERE_PATH)/test_ts2vec_losses.cpp
	@$(COMMON_TORCH_COMPILE_CMD)
	$(RUN_MODE) $(TEST_BUILD)/test_ts2vec_losses
test_ts2vec_losses: $(TEST_BUILD)/test_ts2vec_losses
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_ts2vec_dilated_conv test_ts2vec_dilated_conv
$(TEST_BUILD)/test_ts2vec_dilated_conv: $(HERE_PATH)/test_ts2vec_dilated_conv.cpp
	@$(COMMON_TORCH_COMPILE_CMD)
	$(RUN_MODE) $(TEST_BUILD)/test_ts2vec_dilated_conv
test_ts2vec_dilated_conv: $(TEST_BUILD)/test_ts2vec_dilated_conv
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_ts2vec_encoder test_ts2vec_encoder
$(TEST_BUILD)/test_ts2vec_encoder: $(HERE_PATH)/test_ts2vec_encoder.cpp
	$(COMMON_TORCH_COMPILE_CMD)
	$(RUN_MODE) $(TEST_BUILD)/test_ts2vec_encoder
test_ts2vec_encoder: $(TEST_BUILD)/test_ts2vec_encoder
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_ts2vec_state_dict test_ts2vec_state_dict
$(TEST_BUILD)/test_ts2vec_state_dict: $(HERE_PATH)/test_ts2vec_state_dict.cpp
	@$(COMMON_TORCH_COMPILE_CMD)
	$(RUN_MODE) $(TEST_BUILD)/test_ts2vec_state_dict
test_ts2vec_state_dict: $(TEST_BUILD)/test_ts2vec_state_dict
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_ts2vec_clonable test_ts2vec_clonable
$(TEST_BUILD)/test_ts2vec_clonable: $(HERE_PATH)/test_ts2vec_clonable.cpp
	@$(COMMON_TORCH_COMPILE_CMD)
	$(RUN_MODE) $(TEST_BUILD)/test_ts2vec_clonable
test_ts2vec_clonable: $(TEST_BUILD)/test_ts2vec_clonable
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_ts2vec test_ts2vec
$(TEST_BUILD)/test_ts2vec: $(HERE_PATH)/test_ts2vec.cpp
	@$(COMMON_TORCH_COMPILE_CMD)
	$(RUN_MODE) $(TEST_BUILD)/test_ts2vec
test_ts2vec: $(TEST_BUILD)/test_ts2vec
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_ts2vec_ECG200 test_ts2vec_ECG200
$(TEST_BUILD)/test_ts2vec_ECG200: $(HERE_PATH)/test_ts2vec_ECG200.cpp
	@$(COMMON_TORCH_COMPILE_CMD)
	$(RUN_MODE) $(TEST_BUILD)/test_ts2vec_ECG200
test_ts2vec_ECG200: $(TEST_BUILD)/test_ts2vec_ECG200
# -----------------------------------------------------------------
# - - - General Target -- - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: all
all: $(TEST_BUILD)/test_soft_dtw
	@$(LOG_SUCCESS)
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------