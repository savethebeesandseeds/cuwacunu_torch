# =============================================================================
# tests/bench/wikimyei/backup/enviroment/Makefile — capital allocation strategy
# =============================================================================
# 0) Root & Config
ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config

# 1) Local Paths
HERE_PATH  := $(TESTS_PATH)/bench/wikimyei/backup/enviroment
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

# 2) Defaults (libs/flags)
TEST_DEFAULT_LDLIBS :=

# 3) Tests (build + run aliases)
$(eval $(call TEST_ONEFILE, test_capital_alocation_strategy, test_capital_alocation_strategy.cpp, $(libtorch_a) $(libcommon_a) $(LDLIBS_torch)))

# 4) Target-specific includes/link flags
$(TEST_OUT)/test_capital_alocation_strategy: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)

# 5) Aggregate: all
.PHONY: all
all: $(TEST_OUT)/test_capital_alocation_strategy
	@$(LOG_SUCCESS)

# 6) Aggregate: run
.PHONY: run
run: run-test_capital_alocation_strategy

# 7) Clean
.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/test_capital_alocation_strategy
