# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------
ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config

# -----------------------------------------------------------------------------
# Local Paths
# -----------------------------------------------------------------------------
HERE_PATH  := $(TESTS_PATH)/bench/wikimyei/enviroment
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

# -----------------------------------------------------------------------------
# Build + run aliases
# -----------------------------------------------------------------------------
$(eval $(call TEST_ONEFILE,test_capital_alocation_strategy, test_capital_alocation_strategy.cpp))

# ---- Attach heavy deps (Torch/SSL/cURL) to this test ------------------------
# Headers for compile:
$(TEST_OUT)/test_capital_alocation_strategy: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)

# Link order: torch/curl (consumers) before common (provider)
TEST_DEFAULT_LDLIBS :=
$(TEST_OUT)/test_capital_alocation_strategy: LDLIBS_EXTRA += \
    $(libtorch_a) $(libcommon_a) \
    $(LDLIBS_torch) -lnvToolsExt -lcudart

# -----------------------------------------------------------------------------
# Aggregate build
# -----------------------------------------------------------------------------
.PHONY: all
all: \
	$(OUTPUT_PATH)/test_capital_alocation_strategy
	@$(LOG_SUCCESS)

# -----------------------------------------------------------------------------
# Aggregate run
# -----------------------------------------------------------------------------
.PHONY: run
run: \
     run-test_capital_alocation_strategy

# -----------------------------------------------------------------------------
# Clean
# -----------------------------------------------------------------------------
.PHONY: clean
clean:
	@rm -f $(OUTPUT_PATH)/test_capital_alocation_strategy
