# =============================================================================
# tests/bench/jkimyei/training_setup/Makefile — Torch setup smoke test(s)
# =============================================================================
# 0) Root & Config
ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config

# 1) Local Paths
HERE_PATH  := $(TESTS_PATH)/bench/jkimyei/training_setup
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

# 2) Defaults (libs/flags)
TEST_DEFAULT_LDLIBS :=

# 3) Tests (build + run aliases)
$(eval $(call TEST_ONEFILE, test_jk_setup, test_jk_setup.cpp))

# 4) Target-specific includes/link flags
$(TEST_OUT)/test_jk_setup: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/test_jk_setup: LDLIBS_EXTRA  += $(libtorch_a) $(libcommon_a) $(LDLIBS_torch)

# 5) Aggregate: all
.PHONY: all
all: $(TEST_OUT)/test_jk_setup
	@$(LOG_SUCCESS)

# 6) Aggregate: run
.PHONY: run
run: run-test_jk_setup

# 7) Clean
.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/test_jk_setup
