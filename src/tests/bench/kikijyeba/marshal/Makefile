ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config

HERE_PATH  := $(TESTS_PATH)/bench/kikijyeba/marshal
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

$(eval $(call TEST_ONEFILE, test_kikijyeba_marshal_dispatch, test_kikijyeba_marshal_dispatch.cpp))
$(eval $(call TEST_ONEFILE, test_kikijyeba_marshal_performance_budget, test_kikijyeba_marshal_performance_budget.cpp))

.PHONY: all
all: $(TEST_OUT)/test_kikijyeba_marshal_dispatch $(TEST_OUT)/test_kikijyeba_marshal_performance_budget
	@$(LOG_SUCCESS)

.PHONY: run
run: run-test_kikijyeba_marshal_dispatch run-test_kikijyeba_marshal_performance_budget

.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/test_kikijyeba_marshal_dispatch $(TEST_OUT)/test_kikijyeba_marshal_performance_budget
