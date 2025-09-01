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
$(eval $(call TEST_ONEFILE,test_mixture_density_network_train, test_mixture_density_network_train.cpp))

# ---- Attach Torch/SSL/cURL deps to the test ---------------------------------
# Headers for compile:
$(TEST_OUT)/test_mixture_density_network:	INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/test_mixture_density_network_train: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)

# Link order: torch/curl first, then common (provider)
TEST_DEFAULT_LDLIBS :=

COMMON_LINK = $(libtorch_a) $(libcurl_a) $(libssl_a) $(libcommon_a) \
              $(LDLIBS_torch) $(LDLIBS_curl) $(LDLIBS_ssl)


$(TEST_OUT)/test_mixture_density_network: LDLIBS_EXTRA += $(COMMON_LINK)
$(TEST_OUT)/test_mixture_density_network_train: LDLIBS_EXTRA += $(COMMON_LINK)
# -----------------------------------------------------------------------------
# Aggregate build
# -----------------------------------------------------------------------------
.PHONY: all
all: \
	$(TEST_OUT)/test_mixture_density_network \
	$(TEST_OUT)/test_mixture_density_network_train
	@$(LOG_SUCCESS)

# -----------------------------------------------------------------------------
# Aggregate run
# -----------------------------------------------------------------------------
.PHONY: run
run: \
     run-test_mixture_density_network  \
     run-test_mixture_density_network_train

# -----------------------------------------------------------------------------
# Clean
# -----------------------------------------------------------------------------
.PHONY: clean
clean:
	@rm -f \
	  $(TEST_OUT)/test_mixture_density_network  \
	  $(TEST_OUT)/test_mixture_density_network_train
