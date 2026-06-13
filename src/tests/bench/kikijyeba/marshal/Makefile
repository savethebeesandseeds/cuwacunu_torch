ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config

HERE_PATH  := $(TESTS_PATH)/bench/kikijyeba/marshal
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

$(eval $(call TEST_ONEFILE, test_kikijyeba_marshal_dispatch, test_kikijyeba_marshal_dispatch.cpp))
$(eval $(call TEST_ONEFILE, test_kikijyeba_marshal_evaluation, test_kikijyeba_marshal_evaluation.cpp))
$(eval $(call TEST_ONEFILE, test_kikijyeba_marshal_performance_budget, test_kikijyeba_marshal_performance_budget.cpp, \
  $(LDLIBS_torch)))
$(eval $(call TEST_ONEFILE, test_kikijyeba_marshal_rollout, test_kikijyeba_marshal_rollout.cpp))

.PHONY: hero_mcp_for_marshal
hero_mcp_for_marshal:
	$(MAKE) -C $(SRC_ROOT)/main/hero build-runtime-hero build-environment-hero build-lattice-hero

$(TEST_OUT)/test_kikijyeba_marshal_dispatch: hero_mcp_for_marshal
$(TEST_OUT)/test_kikijyeba_marshal_rollout: hero_mcp_for_marshal

.PHONY: all
all: $(TEST_OUT)/test_kikijyeba_marshal_dispatch $(TEST_OUT)/test_kikijyeba_marshal_evaluation $(TEST_OUT)/test_kikijyeba_marshal_performance_budget $(TEST_OUT)/test_kikijyeba_marshal_rollout
	@$(LOG_SUCCESS)

.PHONY: run
run: run-test_kikijyeba_marshal_dispatch run-test_kikijyeba_marshal_evaluation run-test_kikijyeba_marshal_performance_budget run-test_kikijyeba_marshal_rollout

.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/test_kikijyeba_marshal_dispatch $(TEST_OUT)/test_kikijyeba_marshal_evaluation $(TEST_OUT)/test_kikijyeba_marshal_performance_budget $(TEST_OUT)/test_kikijyeba_marshal_rollout
