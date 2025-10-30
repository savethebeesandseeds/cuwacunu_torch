# =============================================================================
# tests/bench/wikimyei/heuristics/semantic_learning/expected_value/Makefile
# — Expected value model
# =============================================================================

# 0) Root & Config
ROOT_PATH := ../../../../../..
include $(ROOT_PATH)/Makefile.config

# 1) Local Paths
HERE_PATH  := $(TESTS_PATH)/bench/wikimyei/heuristics/semantic_learning/expected_value
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

# 2) Defaults (libs/flags)
# Override to avoid auto-adding libcommon.a twice; we explicitly include it below.
TEST_DEFAULT_LDLIBS :=

# 3) Tests (build + run aliases)
COMMON_LINK = \
  $(libtorch_a) $(libcurl_a) $(libssl_a) $(libcommon_a) \
  $(LDLIBS_torch) $(LDLIBS_curl) $(LDLIBS_ssl)

$(eval $(call TEST_ONEFILE, test_expected_value, test_expected_value.cpp, $(COMMON_LINK)))

# 4) Target-specific includes
$(TEST_OUT)/test_expected_value: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)

# 5) Aggregate: all
.PHONY: all
all: $(TEST_OUT)/test_expected_value
	@$(LOG_SUCCESS)

# 6) Aggregate: run
.PHONY: run
run: run-test_expected_value

# 7) Clean
.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/test_expected_value
