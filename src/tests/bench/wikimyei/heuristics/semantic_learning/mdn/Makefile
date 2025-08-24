# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------
ROOT_PATH := ../../../../../..
include $(ROOT_PATH)/Makefile.config

# -----------------------------------------------------------------------------
# Local Paths
# -----------------------------------------------------------------------------
HERE_PATH  := $(TESTS_PATH)/bench/wikimyei/heuristics/semantic_learning/mdn
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

# -----------------------------------------------------------------------------
# Build + run aliases
# -----------------------------------------------------------------------------
$(eval $(call TEST_ONEFILE,test_mixture_density_network, test_mixture_density_network.cpp))

# ---- Attach Torch/SSL/cURL deps to the test ---------------------------------
# Headers for compile:
$(TEST_OUT)/test_mixture_density_network: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)

# Link order: torch/curl first, then common (provider)
TEST_DEFAULT_LDLIBS :=
$(TEST_OUT)/test_mixture_density_network: LDLIBS_EXTRA += \
    $(libtorch_a) $(libcommon_a) \
    $(LDLIBS_torch) -lnvToolsExt -lcudart

# -----------------------------------------------------------------------------
# Aggregate build
# -----------------------------------------------------------------------------
.PHONY: all
all: \
	$(TEST_OUT)/test_mixture_density_network
	@$(LOG_SUCCESS)

# -----------------------------------------------------------------------------
# Aggregate run
# -----------------------------------------------------------------------------
.PHONY: run
run: \
     run-test_mixture_density_network

# -----------------------------------------------------------------------------
# Clean
# -----------------------------------------------------------------------------
.PHONY: clean
clean:
	@rm -f \
	  $(TEST_OUT)/test_mixture_density_network
