# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config

# -----------------------------------------------------------------------------
# Local Paths
# -----------------------------------------------------------------------------
HERE_PATH  := $(TESTS_PATH)/bench/wikimyei
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

# -----------------------------------------------------------------------------
# Build + run aliases
# -----------------------------------------------------------------------------
$(eval $(call TEST_ONEFILE,test_entropy_methods, test_entropy_methods.cpp))

# ---- Extra headers / libs ----------------------------------------------------
# Headers for compile:
$(TEST_OUT)/test_entropy_methods: INCLUDES_EXTRA += $(SSL_INCLUDE_PATHS)

# Link order: deps first, then common/provider
TEST_DEFAULT_LDLIBS :=
$(TEST_OUT)/test_entropy_methods: LDLIBS_EXTRA += \
    $(libcommon_a) \
    $(LIBCURL_LD_LAGS) $(SSL_LD_FLAGS)

# -----------------------------------------------------------------------------
# Aggregate build
# -----------------------------------------------------------------------------
.PHONY: all
all: \
	$(TEST_OUT)/test_entropy_methods
	$(MAKE) -C $(HERE_PATH)/heuristics all
	@$(LOG_SUCCESS)

# -----------------------------------------------------------------------------
# Aggregate run
# -----------------------------------------------------------------------------
.PHONY: run
run: \
     run-test_entropy_methods

# -----------------------------------------------------------------------------
# Clean
# -----------------------------------------------------------------------------
.PHONY: clean
clean:
	@rm -f \
	  $(TEST_OUT)/test_entropy_methods
