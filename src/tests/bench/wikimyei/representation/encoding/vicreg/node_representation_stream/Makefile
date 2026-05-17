ROOT_PATH := ../../../../../../..
include $(ROOT_PATH)/Makefile.config

HERE_PATH  := $(TESTS_PATH)/bench/wikimyei/representation/encoding/vicreg/node_representation_stream
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

TEST_DEFAULT_LDLIBS :=

NODE_REPRESENTATION_STREAM_OBJS := \
  $(OUTPUT_PATH)/common/core/utils.o \
  $(OUTPUT_PATH)/common/io/files.o \
  $(OUTPUT_PATH)/common/parse/json/json_parsing.o \
  $(OUTPUT_PATH)/common/statistics_space.o \
  $(OUTPUT_PATH)/common/types_data.o \
  $(OUTPUT_PATH)/common/types_utils.o \
  $(OUTPUT_PATH)/common/source_contract.o \
  $(OUTPUT_PATH)/common/source_contract_decode.o \
  $(OUTPUT_PATH)/common/source_forms_decoder.o \
  $(OUTPUT_PATH)/common/channel_forms_decoder.o \
  $(OUTPUT_PATH)/common/graph_forms_decoder.o \
  $(OUTPUT_PATH)/common/parser_types.o \
  $(OUTPUT_PATH)/common/ast.o \
  $(OUTPUT_PATH)/common/grammar_lexer.o \
  $(OUTPUT_PATH)/common/grammar_parser.o \
  $(OUTPUT_PATH)/common/instruction_lexer.o \
  $(OUTPUT_PATH)/common/instruction_parser.o \
  $(OUTPUT_PATH)/common/memory_mapped_datafile.o \
  $(OUTPUT_PATH)/libtorch/memory_mapped_dataset.o \
  $(OUTPUT_PATH)/libtorch/memory_mapped_dataloader.o

.PHONY: node_representation_stream_objects
node_representation_stream_objects:
	$(MAKE) -C $(IMPL_PATH)/piaabo utils files json_parsing
	$(MAKE) -C $(IMPL_PATH)/piaabo/parse/bnf all
	$(MAKE) -C $(IMPL_PATH)/piaabo/math all
	$(MAKE) -C $(IMPL_PATH)/ujcamei/source/types all
	$(MAKE) -C $(IMPL_PATH)/ujcamei/source/contract all
	$(MAKE) -C $(IMPL_PATH)/ujcamei/source/storage/memory_mapped all

$(eval $(call TEST_ONEFILE, test_wikimyei_vicreg_node_representation_stream, test_wikimyei_vicreg_node_representation_stream.cpp, \
  $(libtorch_a) $(NODE_REPRESENTATION_STREAM_OBJS) $(LDLIBS_torch)))

$(TEST_OUT)/test_wikimyei_vicreg_node_representation_stream: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/test_wikimyei_vicreg_node_representation_stream: node_representation_stream_objects

.PHONY: all
all: $(TEST_OUT)/test_wikimyei_vicreg_node_representation_stream
	@$(LOG_SUCCESS)

.PHONY: run
run: node_representation_stream_objects run-test_wikimyei_vicreg_node_representation_stream

.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/test_wikimyei_vicreg_node_representation_stream
