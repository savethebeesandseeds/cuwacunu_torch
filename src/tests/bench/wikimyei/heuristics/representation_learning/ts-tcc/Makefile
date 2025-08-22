# -----------------------------------------------------------------
# Dependencies - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
ROOT_PATH := ../../../../..
include $(ROOT_PATH)/Makefile.config
# -----------------------------------------------------------------
# Local Makefile Variables - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
HERE_PATH := $(ROOT_PATH)/tests/bench/wikimyei/heuristics/ts-tcc
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
.PHONY: $(TEST_BUILD)/test_soft_dtw test_soft_dtw
$(TEST_BUILD)/test_soft_dtw: $(HERE_PATH)/test_soft_dtw.cpp
	@$(COMMON_TORCH_COMPILE_CMD)
	$(RUN_MODE) $(TEST_BUILD)/test_soft_dtw
test_soft_dtw: $(TEST_BUILD)/test_soft_dtw
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_time_contrastive_coding test_time_contrastive_coding
$(TEST_BUILD)/test_time_contrastive_coding: $(HERE_PATH)/test_time_contrastive_coding.cpp
	@$(COMMON_TORCH_COMPILE_CMD)
	$(RUN_MODE) $(TEST_BUILD)/test_time_contrastive_coding
test_time_contrastive_coding: $(TEST_BUILD)/test_time_contrastive_coding
# -----------------------------------------------------------------
# - - - General Target -- - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: all
all: $(TEST_BUILD)/test_soft_dtw
	@$(LOG_SUCCESS)
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------