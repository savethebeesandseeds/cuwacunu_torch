# =============================================================================
# tests/bench/piaabo/torch_compat/Makefile — (placeholder) Torch compat tests
# =============================================================================
# 0) Root & Config
ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config

# 1) Local Paths
HERE_PATH  := $(TESTS_PATH)/bench/piaabo/torch_compat
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

# 2) Defaults (libs/flags)
TEST_DEFAULT_LDLIBS :=

# 3) Tests (build + run aliases)
$(eval $(call TEST_ONEFILE, test_cuda_probe, test_cuda_probe.cpp, $(LDLIBS_torch)))

# 4) Target-specific includes/link flags
$(TEST_OUT)/test_cuda_probe: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)

# 5) Aggregate: all
.PHONY: all
all: $(TEST_OUT)/test_cuda_probe
	@$(LOG_SUCCESS)

# 6) Aggregate: run
.PHONY: run
run: run-test_cuda_probe

# 7) Clean
.PHONY: clean
clean:
	$(Q)$(CLEAN_RM_CMD) $(TEST_OUT)/test_cuda_probe
