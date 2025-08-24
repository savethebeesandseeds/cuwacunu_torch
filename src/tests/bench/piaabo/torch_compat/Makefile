# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------
ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config

# -----------------------------------------------------------------------------
# Local Paths
# -----------------------------------------------------------------------------
HERE_PATH  := $(TESTS_PATH)/bench/piaabo/torch_compat
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

# -----------------------------------------------------------------------------
# Build + run aliases
# -----------------------------------------------------------------------------
$(eval $(call TEST_ONEFILE,test_gpu_vs_cpu, test_gpu_vs_cpu.cpp))

# ---- Attach Torch/SSL/cURL deps (headers + archives + system libs) ----------
# Headers for compile:
$(TEST_OUT)/test_gpu_vs_cpu: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)

# Link order: consumer libs first; disable default so we control ordering
TEST_DEFAULT_LDLIBS :=
$(TEST_OUT)/test_gpu_vs_cpu: LDLIBS_EXTRA += \
    $(libtorch_a) $(libcommon_a) \
    $(LDLIBS_torch)

# -----------------------------------------------------------------------------
# Aggregate build
# -----------------------------------------------------------------------------
.PHONY: all
all: \
	$(OUTPUT_PATH)/test_gpu_vs_cpu
	@$(LOG_SUCCESS)

# -----------------------------------------------------------------------------
# Aggregate run
# -----------------------------------------------------------------------------
.PHONY: run
run: \
     run-test_gpu_vs_cpu

# -----------------------------------------------------------------------------
# Clean
# -----------------------------------------------------------------------------
.PHONY: clean
clean:
	@rm -f $(OUTPUT_PATH)/test_gpu_vs_cpu
