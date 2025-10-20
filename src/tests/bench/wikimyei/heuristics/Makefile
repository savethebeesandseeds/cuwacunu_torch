# =============================================================================
# tests/bench/wikimyei/heuristics/Makefile — heuristics aggregator
# =============================================================================
# 0) Root & Config
ROOT_PATH := ../../../../..
include $(ROOT_PATH)/Makefile.config

# 1) Local Paths
HERE_PATH := $(TESTS_PATH)/bench/wikimyei/heuristics
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

# 2) Defaults (libs/flags)
# (none)

# 3) Tests (build + run aliases)
# (none — aggregator)

# 4) Target-specific includes/link flags
# (none)

# 5) Aggregate: all
.PHONY: all
all:
	+$(MAKE) -C $(HERE_PATH)/representation_learning all
	@$(LOG_SUCCESS)
