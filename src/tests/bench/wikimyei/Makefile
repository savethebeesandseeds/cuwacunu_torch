# =============================================================================
# tests/bench/wikimyei/Makefile — wikimyei entrypoint + entropy test
# =============================================================================
# 0) Root & Config
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config

# 1) Local Paths
HERE_PATH  := $(TESTS_PATH)/bench/wikimyei
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

# 2) Defaults (libs/flags)
# (none)

# 3) Tests (build + run aliases)
# (none)

# 4) Target-specific includes/link flags
# (none)

# 5) Aggregate: all
.PHONY: all
all: \
	+$(MAKE) -C $(HERE_PATH)/representation all
	+$(MAKE) -C $(HERE_PATH)/inference all
	@$(LOG_SUCCESS)

# 6) Aggregate: run
# (none)

# 7) Clean
.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/test_entropy_methods

