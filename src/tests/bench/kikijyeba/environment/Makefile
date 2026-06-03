ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config

HERE_PATH  := $(TESTS_PATH)/bench/kikijyeba/environment
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

TEST_DEFAULT_LDLIBS :=

$(eval $(call TEST_ONEFILE, test_kikijyeba_environment_contract, test_kikijyeba_environment_contract.cpp, \
  $(libtorch_a) $(LDLIBS_torch)))

$(TEST_OUT)/test_kikijyeba_environment_contract: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)

.PHONY: all
all: $(TEST_OUT)/test_kikijyeba_environment_contract
	@$(LOG_SUCCESS)

.PHONY: run
run: run-test_kikijyeba_environment_contract

.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/test_kikijyeba_environment_contract
