ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config

HERE_PATH  := $(TESTS_PATH)/bench/kikijyeba/environment
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

TEST_DEFAULT_LDLIBS :=

$(OUTPUT_PATH)/common/hero_environment_tools.o: \
	$(IMPL_PATH)/hero/environment_hero/hero_environment_tools.cpp \
	$(SRC_ROOT)/Makefile.config
	$(CC_RULE)

$(eval $(call TEST_ONEFILE, test_kikijyeba_environment_contract, test_kikijyeba_environment_contract.cpp, \
  $(libtorch_a) $(LDLIBS_torch)))
$(eval $(call TEST_ONEFILE, test_hero_environment_rollout_direct, test_hero_environment_rollout_direct.cpp, \
  $(OUTPUT_PATH)/common/hero_environment_tools.o))

$(TEST_OUT)/test_kikijyeba_environment_contract: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)

.PHONY: all
all: $(TEST_OUT)/test_kikijyeba_environment_contract $(TEST_OUT)/test_hero_environment_rollout_direct
	@$(LOG_SUCCESS)

.PHONY: run
run: run-test_kikijyeba_environment_contract run-test_hero_environment_rollout_direct

.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/test_kikijyeba_environment_contract $(TEST_OUT)/test_hero_environment_rollout_direct
