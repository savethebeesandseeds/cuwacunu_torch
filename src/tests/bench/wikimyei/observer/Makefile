ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config

HERE_PATH  := $(TESTS_PATH)/bench/wikimyei/observer
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

TEST_DEFAULT_LDLIBS :=

$(eval $(call TEST_ONEFILE, test_wikimyei_observer_portfolio, test_wikimyei_observer_portfolio.cpp, \
  $(libtorch_a) $(LDLIBS_torch)))

$(TEST_OUT)/test_wikimyei_observer_portfolio: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)

.PHONY: all
all: $(TEST_OUT)/test_wikimyei_observer_portfolio
	@$(LOG_SUCCESS)

.PHONY: run
run: run-test_wikimyei_observer_portfolio

.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/test_wikimyei_observer_portfolio
