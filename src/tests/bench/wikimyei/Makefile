# =============================================================================
# tests/bench/wikimyei/Makefile — flattened wikimyei bench tests
# =============================================================================
# 0) Root & Config
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config

# 1) Local Paths
HERE_PATH  := $(TESTS_PATH)/bench/wikimyei
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

# 2) Defaults (libs/flags)
TEST_DEFAULT_LDLIBS :=

# 3) Tests (build + run aliases)
COMMON_LINK = $(libtorch_a) $(libcurl_a) $(libssl_a) $(libcommon_a) $(LDLIBS_torch) $(LDLIBS_curl) $(LDLIBS_ssl)

$(eval $(call TEST_ONEFILE, test_entropy_methods,                 test_entropy_methods.cpp,                 $(libcommon_a)))
$(eval $(call TEST_ONEFILE, test_vicreg_4d_projector_gpu_vs_cpu, test_vicreg_4d_projector_gpu_vs_cpu.cpp, $(COMMON_LINK)))
$(eval $(call TEST_ONEFILE, test_vicreg_4d_train,         test_vicreg_4d_train.cpp,         $(COMMON_LINK)))
$(eval $(call TEST_ONEFILE, test_vicreg_4d_save_and_load, test_vicreg_4d_save_and_load.cpp, $(COMMON_LINK)))
$(eval $(call TEST_ONEFILE, test_mixture_density_network,         test_mixture_density_network.cpp,         $(COMMON_LINK)))
$(eval $(call TEST_ONEFILE, test_expected_value,                  test_expected_value.cpp,                  $(COMMON_LINK)))

# 4) Target-specific includes/link flags
$(TEST_OUT)/test_vicreg_%: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS)
$(TEST_OUT)/test_mixture_density_network: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS)
$(TEST_OUT)/test_expected_value: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)

# 5) Aggregate: all
.PHONY: all
all: \
	$(TEST_OUT)/test_entropy_methods \
	$(TEST_OUT)/test_vicreg_4d_projector_gpu_vs_cpu \
	$(TEST_OUT)/test_vicreg_4d_train \
	$(TEST_OUT)/test_vicreg_4d_save_and_load \
	$(TEST_OUT)/test_mixture_density_network \
	$(TEST_OUT)/test_expected_value
	@$(LOG_SUCCESS)

# 6) Aggregate: run
.PHONY: run
run: \
	run-test_entropy_methods \
	run-test_vicreg_4d_projector_gpu_vs_cpu \
	run-test_vicreg_4d_train \
	run-test_vicreg_4d_save_and_load \
	run-test_mixture_density_network \
	run-test_expected_value

# 7) Clean
.PHONY: clean
clean:
	@rm -f \
	  $(TEST_OUT)/test_entropy_methods \
	  $(TEST_OUT)/test_vicreg_4d_projector_gpu_vs_cpu \
	  $(TEST_OUT)/test_vicreg_4d_train \
	  $(TEST_OUT)/test_vicreg_4d_save_and_load \
	  $(TEST_OUT)/test_mixture_density_network \
	  $(TEST_OUT)/test_expected_value
