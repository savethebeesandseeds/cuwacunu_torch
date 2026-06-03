ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config

HERE_PATH  := $(TESTS_PATH)/bench/cajtucu/execution
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

TEST_DEFAULT_LDLIBS :=

$(eval $(call TEST_ONEFILE, test_cajtucu_execution_paper, test_cajtucu_execution_paper.cpp, \
  $(libtorch_a) $(LDLIBS_torch)))

$(TEST_OUT)/test_cajtucu_execution_paper: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)

.PHONY: all
all: $(TEST_OUT)/test_cajtucu_execution_paper
	@$(LOG_SUCCESS)

.PHONY: run
run: run-test_cajtucu_execution_paper

.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/test_cajtucu_execution_paper

