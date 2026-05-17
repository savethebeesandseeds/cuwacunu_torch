ROOT_PATH := ../../../../..
include $(ROOT_PATH)/Makefile.config

HERE_PATH  := $(TESTS_PATH)/bench/ujcamei/graph/market_graph
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

TEST_DEFAULT_LDLIBS :=

$(eval $(call TEST_ONEFILE, test_ujcamei_market_graph, test_ujcamei_market_graph.cpp, ))

.PHONY: all
all: $(TEST_OUT)/test_ujcamei_market_graph
	@$(LOG_SUCCESS)

.PHONY: run
run: run-test_ujcamei_market_graph

.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/test_ujcamei_market_graph
