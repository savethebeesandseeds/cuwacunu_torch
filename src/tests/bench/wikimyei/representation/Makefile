# =============================================================================
# tests/bench/wikimyei/representation/Makefile — RL agg
# =============================================================================
# 0) Root & Config
ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config

# 1) Local Paths
HERE_PATH := $(TESTS_PATH)/bench/wikimyei/representation
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
	+$(MAKE) -C $(HERE_PATH)/VICReg all
	@$(LOG_SUCCESS)
