# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------
ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config

# -----------------------------------------------------------------------------
# Local Paths
# -----------------------------------------------------------------------------
HERE_PATH  := $(TESTS_PATH)/bench/jkimyei/training_components
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

# -----------------------------------------------------------------------------
# Build + run aliases
# -----------------------------------------------------------------------------
$(eval $(call TEST_ONEFILE,test_training_components_jk_setup, test_training_components_jk_setup.cpp))

# ---- Attach heavy deps (Torch/SSL/cURL) to this test ------------------------
# Headers for compile:
$(TEST_OUT)/test_training_components_jk_setup: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)

# Link order: torch (consumer) before ssl/curl/common
TEST_DEFAULT_LDLIBS :=
$(TEST_OUT)/test_training_components_jk_setup: LDLIBS_EXTRA += \
    $(libtorch_a) $(libcommon_a) \
    $(LDLIBS_torch) -lnvToolsExt -lcudart

# -----------------------------------------------------------------------------
# Aggregate build
# -----------------------------------------------------------------------------
.PHONY: all
all: \
	$(OUTPUT_PATH)/test_training_components_jk_setup
	@$(LOG_SUCCESS)

# -----------------------------------------------------------------------------
# Aggregate run
# -----------------------------------------------------------------------------
.PHONY: run
run: \
     run-test_training_components_jk_setup

# -----------------------------------------------------------------------------
# Clean
# -----------------------------------------------------------------------------
.PHONY: clean
clean:
	@rm -f $(OUTPUT_PATH)/test_training_components_jk_setup
