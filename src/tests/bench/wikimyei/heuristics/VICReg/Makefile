# -----------------------------------------------------------------
# Dependencies - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
ROOT_PATH := ../../../../..
include $(ROOT_PATH)/Makefile.config
# -----------------------------------------------------------------
# Local Makefile Variables - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
HERE_PATH := $(ROOT_PATH)/tests/bench/wikimyei/heuristics/VICReg
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
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_vicreg_4d_observation_pipeline test_vicreg_4d_observation_pipeline
$(TEST_BUILD)/test_vicreg_4d_observation_pipeline: $(HERE_PATH)/test_vicreg_4d_observation_pipeline.cpp
	@$(COMMON_TORCH_COMPILE_CMD)
	$(RUN_MODE) $(TEST_BUILD)/test_vicreg_4d_observation_pipeline
test_vicreg_4d_observation_pipeline: $(TEST_BUILD)/test_vicreg_4d_observation_pipeline
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_vicreg_4d_projector_gpu_vs_cpu test_vicreg_4d_projector_gpu_vs_cpu
$(TEST_BUILD)/test_vicreg_4d_projector_gpu_vs_cpu: $(HERE_PATH)/test_vicreg_4d_projector_gpu_vs_cpu.cpp
	@$(COMMON_TORCH_COMPILE_CMD)
	$(RUN_MODE) $(TEST_BUILD)/test_vicreg_4d_projector_gpu_vs_cpu
test_vicreg_4d_projector_gpu_vs_cpu: $(TEST_BUILD)/test_vicreg_4d_projector_gpu_vs_cpu
# -----------------------------------------------------------------
# - - - General Target -- - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: all
all: $(TEST_BUILD)/test_vicreg_4d_observation_pipeline
	@$(LOG_SUCCESS)
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------