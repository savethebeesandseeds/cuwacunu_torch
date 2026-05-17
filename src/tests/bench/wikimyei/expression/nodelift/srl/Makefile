ROOT_PATH := ../../../../../..
include $(ROOT_PATH)/Makefile.config

HERE_PATH  := $(TESTS_PATH)/bench/wikimyei/expression/nodelift/srl
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

TEST_DEFAULT_LDLIBS :=

$(eval $(call TEST_ONEFILE, test_wikimyei_nodelift_srl, test_wikimyei_nodelift_srl.cpp, \
  $(libtorch_a) $(LDLIBS_torch)))

$(eval $(call TEST_ONEFILE, test_wikimyei_nodelift_complexity, test_wikimyei_nodelift_complexity.cpp, \
  $(libtorch_a) $(LDLIBS_torch)))

$(TEST_OUT)/test_wikimyei_nodelift_srl: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/test_wikimyei_nodelift_complexity: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)

.PHONY: all
all: $(TEST_OUT)/test_wikimyei_nodelift_srl $(TEST_OUT)/test_wikimyei_nodelift_complexity
	@$(LOG_SUCCESS)

.PHONY: run
run: run-test_wikimyei_nodelift_srl run-test_wikimyei_nodelift_complexity

.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/test_wikimyei_nodelift_srl $(TEST_OUT)/test_wikimyei_nodelift_complexity
