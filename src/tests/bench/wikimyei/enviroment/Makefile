# -----------------------------------------------------------------
# Dependencies - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config
# -----------------------------------------------------------------
# Local Makefile Variables - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
HERE_PATH := $(ROOT_PATH)/tests/bench/wikimyei/enviroment
TEST_BUILD := $(ROOT_PATH)/tests/build
COMMON_TORCH_COMPILE_CMD = \
    $(LOG_START) && $(LOCAL_CLEAN) && \
    $(GXX) $(GXX_FLAGS) \
      $(TORCH_INCLUDE_PATHS) \
      $(SSL_INCLUDE_PATHS) \
      $(USUAL_INCLUDE_PATHS) \
      $(LIBCURL_INCLUDE_PATHS) \
      $(OUTPUT_PATH)/common/*.o \
      $(OUTPUT_PATH)/libcurl/*.o \
      $(OUTPUT_PATH)/libtorch/*.o \
      $(OUTPUT_PATH)/openssl/*.o \
      $< \
      $(SSL_LD_FLAGS) \
      $(LIBCURL_LD_LAGS) \
      $(TORCH_LD_FLAGS) \
      -lnvToolsExt -lcudart \
      $(USUAL_LD_FLAGS) \
      -o $@ && $(LOG_END)
# -----------------------------------------------------------------
# - - - Local Targets - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_capital_alocation_strategy test_capital_alocation_strategy
$(TEST_BUILD)/test_capital_alocation_strategy: $(HERE_PATH)/test_capital_alocation_strategy.cpp
	@$(COMMON_TORCH_COMPILE_CMD)
	$(RUN_MODE) $(TEST_BUILD)/test_capital_alocation_strategy
test_capital_alocation_strategy: $(TEST_BUILD)/test_capital_alocation_strategy

# -----------------------------------------------------------------
# - - - General Target -- - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: all
all: $(TEST_BUILD)/test_vicreg_4d_observation_pipeline
	@$(LOG_SUCCESS)
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------