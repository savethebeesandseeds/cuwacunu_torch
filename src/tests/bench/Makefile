# =============================================================================
# tests/bench/Makefile — bench-level aggregator for all modules
# =============================================================================
# 0) Root & Config
ROOT_PATH := ../..
include $(ROOT_PATH)/Makefile.config

# 1) Local Paths
HERE_PATH  := $(TESTS_PATH)/bench
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
	+{ \
		$(MAKE) -C $(HERE_PATH)/piaabo all \
		$(MAKE) -C $(HERE_PATH)/camahjucunu all \
		$(MAKE) -C $(HERE_PATH)/jkimyei all \
		$(MAKE) -C $(HERE_PATH)/wikimyei all \
		$(MAKE) -C $(HERE_PATH)/tsiemene all \
	}
	@$(LOG_SUCCESS)

# 6) Aggregate: run
# (not used here)

# 7) Clean
# (use subdir cleans if needed)
