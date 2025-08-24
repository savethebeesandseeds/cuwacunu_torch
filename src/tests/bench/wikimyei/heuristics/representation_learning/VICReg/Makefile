# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------
ROOT_PATH := ../../../../../..
include $(ROOT_PATH)/Makefile.config

# -----------------------------------------------------------------------------
# Local Paths
# -----------------------------------------------------------------------------
HERE_PATH  := $(TESTS_PATH)/bench/wikimyei/heuristics/representation_learning/VICReg
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

# -----------------------------------------------------------------------------
# Build + run aliases
# -----------------------------------------------------------------------------
$(eval $(call TEST_ONEFILE,test_vicreg_4d_observation_pipeline, test_vicreg_4d_observation_pipeline.cpp))
$(eval $(call TEST_ONEFILE,test_vicreg_4d_projector_gpu_vs_cpu, test_vicreg_4d_projector_gpu_vs_cpu.cpp))
$(eval $(call TEST_ONEFILE,test_vicreg_4d_save_and_load,        test_vicreg_4d_save_and_load.cpp))

# ---- Attach Torch/SSL/cURL deps to each test --------------------------------
# ---- Headers (ok to keep as-is) ---------------------------------------------
$(TEST_OUT)/test_vicreg_4d_observation_pipeline: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS)
$(TEST_OUT)/test_vicreg_4d_projector_gpu_vs_cpu: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS)
$(TEST_OUT)/test_vicreg_4d_save_and_load:        INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS)

# ---- Link order: project archives first, then system libs -------------------
TEST_DEFAULT_LDLIBS :=

COMMON_LINK = $(libtorch_a) $(libcurl_a) $(libssl_a) $(libcommon_a) \
              $(LDLIBS_torch) $(LDLIBS_curl) $(LDLIBS_ssl)

$(TEST_OUT)/test_vicreg_4d_observation_pipeline: LDLIBS_EXTRA += $(COMMON_LINK)
$(TEST_OUT)/test_vicreg_4d_projector_gpu_vs_cpu: LDLIBS_EXTRA += $(COMMON_LINK)
$(TEST_OUT)/test_vicreg_4d_save_and_load:        LDLIBS_EXTRA += $(COMMON_LINK)

# (Note: -lnvToolsExt and -lcudart are already in $(LDLIBS_torch); no need to repeat.)

# ----------------------------------------------------------------------------- 
# Aggregate build / clean should use TEST_OUT (not OUTPUT_PATH)
# -----------------------------------------------------------------------------
.PHONY: all
all: \
	$(TEST_OUT)/test_vicreg_4d_observation_pipeline \
	$(TEST_OUT)/test_vicreg_4d_projector_gpu_vs_cpu \
	$(TEST_OUT)/test_vicreg_4d_save_and_load
	@$(LOG_SUCCESS)

.PHONY: clean
clean:
	@rm -f \
	  $(TEST_OUT)/test_vicreg_4d_observation_pipeline \
	  $(TEST_OUT)/test_vicreg_4d_projector_gpu_vs_cpu \
	  $(TEST_OUT)/test_vicreg_4d_save_and_load
