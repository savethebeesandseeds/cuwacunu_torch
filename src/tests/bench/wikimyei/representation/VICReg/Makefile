# =============================================================================
# tests/bench/wikimyei/representation/VICReg/Makefile
# — VICReg tests
# =============================================================================
# 0) Root & Config
ROOT_PATH := ../../../../..
include $(ROOT_PATH)/Makefile.config

# 1) Local Paths
HERE_PATH  := $(TESTS_PATH)/bench/wikimyei/representation/VICReg
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

# 2) Defaults (libs/flags)
TEST_DEFAULT_LDLIBS :=

# 3) Tests (build + run aliases)
COMMON_LINK = $(libtorch_a) $(libcurl_a) $(libssl_a) $(libcommon_a) $(LDLIBS_torch) $(LDLIBS_curl) $(LDLIBS_ssl)
$(eval $(call TEST_ONEFILE, test_vicreg_4d_projector_gpu_vs_cpu, test_vicreg_4d_projector_gpu_vs_cpu.cpp, $(COMMON_LINK)))
# LEGACY-TODO(vicreg-tests): re-enable after migrating to the current
# dataloader factory/template API and VICReg_4D encode/save constructor API.
# $(eval $(call TEST_ONEFILE, test_vicreg_4d_train,         test_vicreg_4d_train.cpp,         $(COMMON_LINK)))
# $(eval $(call TEST_ONEFILE, test_vicreg_4d_save_and_load, test_vicreg_4d_save_and_load.cpp, $(COMMON_LINK)))

# 4) Target-specific includes/link flags
$(TEST_OUT)/test_vicreg_%: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS)

# 5) Aggregate: all
.PHONY: all
all: \
		$(TEST_OUT)/test_vicreg_4d_projector_gpu_vs_cpu
		@$(LOG_SUCCESS)

# 6) Aggregate: run
.PHONY: run
run: \
		run-test_vicreg_4d_projector_gpu_vs_cpu

# 7) Clean
.PHONY: clean
clean:
	@rm -f \
	  $(TEST_OUT)/test_vicreg_4d_projector_gpu_vs_cpu
