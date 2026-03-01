# =============================================================================
# tests/bench/jkimyei/Makefile — jkimyei bench tests
# =============================================================================
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config

HERE_PATH  := $(TESTS_PATH)/bench/jkimyei
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

TEST_DEFAULT_LDLIBS :=

$(eval $(call TEST_ONEFILE, test_jk_setup, test_jk_setup.cpp))

$(TEST_OUT)/test_jk_setup: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/test_jk_setup: LDLIBS_EXTRA  += $(libtorch_a) $(libcommon_a) $(LDLIBS_torch)

.PHONY: all
all: $(TEST_OUT)/test_jk_setup
	@$(LOG_SUCCESS)

.PHONY: run
run: run-test_jk_setup

.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/test_jk_setup

