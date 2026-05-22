ROOT_PATH := ../../../../../..
include $(ROOT_PATH)/Makefile.config

HERE_PATH  := $(TESTS_PATH)/bench/ujcamei/source/retrieval/dataloader/graph_anchor_edge_batch
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

TEST_DEFAULT_LDLIBS :=

$(eval $(call TEST_ONEFILE, test_ujcamei_graph_anchor_edge_batch, test_ujcamei_graph_anchor_edge_batch.cpp, \
  $(libtorch_a) $(LDLIBS_torch)))

$(TEST_OUT)/test_ujcamei_graph_anchor_edge_batch: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)

.PHONY: all
all: $(TEST_OUT)/test_ujcamei_graph_anchor_edge_batch
	@$(LOG_SUCCESS)

.PHONY: run
run: run-test_ujcamei_graph_anchor_edge_batch

.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/test_ujcamei_graph_anchor_edge_batch
