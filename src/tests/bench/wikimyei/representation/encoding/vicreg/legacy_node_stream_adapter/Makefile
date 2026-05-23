ROOT_PATH := ../../../../../../..
include $(ROOT_PATH)/Makefile.config

HERE_PATH  := $(TESTS_PATH)/bench/wikimyei/representation/encoding/vicreg/node_stream_adapter
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

TEST_DEFAULT_LDLIBS :=

$(eval $(call TEST_ONEFILE, test_wikimyei_vicreg_node_stream_adapter, test_wikimyei_vicreg_node_stream_adapter.cpp, \
  $(libtorch_a) $(LDLIBS_torch)))

$(TEST_OUT)/test_wikimyei_vicreg_node_stream_adapter: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)

.PHONY: all
all: $(TEST_OUT)/test_wikimyei_vicreg_node_stream_adapter
	@$(LOG_SUCCESS)

.PHONY: run
run: run-test_wikimyei_vicreg_node_stream_adapter

.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/test_wikimyei_vicreg_node_stream_adapter
