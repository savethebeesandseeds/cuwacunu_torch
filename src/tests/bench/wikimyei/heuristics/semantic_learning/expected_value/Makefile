# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------
ROOT_PATH := ../../../../../..
include $(ROOT_PATH)/Makefile.config

# -----------------------------------------------------------------------------
# Local Paths
# -----------------------------------------------------------------------------
HERE_PATH  := $(TESTS_PATH)/bench/wikimyei/heuristics/semantic_learning/expected_value
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

# -----------------------------------------------------------------------------
# Build + run aliases
# -----------------------------------------------------------------------------
$(eval $(call TEST_ONEFILE,test_expected_value, test_expected_value.cpp))

# ---- Attach Torch/SSL/cURL deps to the test ---------------------------------
# Headers for compile:
$(TEST_OUT)/test_expected_value:	INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)

# Link order: torch/curl first, then common (provider)
TEST_DEFAULT_LDLIBS :=

COMMON_LINK = $(libtorch_a) $(libcurl_a) $(libssl_a) $(libcommon_a) \
              $(LDLIBS_torch) $(LDLIBS_curl) $(LDLIBS_ssl)


$(TEST_OUT)/test_expected_value: LDLIBS_EXTRA += $(COMMON_LINK)
# -----------------------------------------------------------------------------
# Aggregate build
# -----------------------------------------------------------------------------
.PHONY: all
all: \
	$(TEST_OUT)/test_expected_value
	@$(LOG_SUCCESS)

# -----------------------------------------------------------------------------
# Aggregate run
# -----------------------------------------------------------------------------
.PHONY: run
run: \
     run-test_expected_value

# -----------------------------------------------------------------------------
# Clean
# -----------------------------------------------------------------------------
.PHONY: clean
clean:
	@rm -f \
	  $(TEST_OUT)/test_expected_value
