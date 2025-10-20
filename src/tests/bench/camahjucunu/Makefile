# =============================================================================
# tests/bench/camahjucunu/Makefile — module aggregator
# =============================================================================
# 0) Root & Config
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config

# 1) Local Paths
HERE_PATH  := $(TESTS_PATH)/bench/camahjucunu
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

# 2) Defaults (libs/flags)
# (none)

# 3) Tests (build + run aliases)
# (none — this is an aggregator)

# 4) Target-specific includes/link flags
# (none)

# 5) Aggregate: all
.PHONY: all
all:
	+$(MAKE) -C $(HERE_PATH)/https all
	+$(MAKE) -C $(HERE_PATH)/types all
	+$(MAKE) -C $(HERE_PATH)/exchange all
	+$(MAKE) -C $(HERE_PATH)/bnf all
	+$(MAKE) -C $(HERE_PATH)/data all
	@$(LOG_SUCCESS)

# 6) Aggregate: run
# (not used here)

# 7) Clean
# (use subdir cleans if needed)
