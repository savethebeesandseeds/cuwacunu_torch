ROOT_PATH := ../../../../../..
include $(ROOT_PATH)/Makefile.config

HERE_PATH  := $(TESTS_PATH)/bench/ujcamei/source/retrieval/dataloader/graph_anchor_edge_dataset
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

TEST_DEFAULT_LDLIBS :=

MEMORY_MAPPED_SOURCE_OBJS := \
  $(OUTPUT_PATH)/common/core/utils.o \
  $(OUTPUT_PATH)/common/io/files.o \
  $(OUTPUT_PATH)/common/parse/json/json_parsing.o \
  $(OUTPUT_PATH)/common/statistics_space.o \
  $(OUTPUT_PATH)/common/registry_data.o \
  $(OUTPUT_PATH)/common/registry_utils.o \
  $(OUTPUT_PATH)/common/source_contract.o \
  $(OUTPUT_PATH)/common/source_contract_decode.o \
  $(OUTPUT_PATH)/common/source_registry_decoder.o \
  $(OUTPUT_PATH)/common/retrieval_channel_decoder.o \
  $(OUTPUT_PATH)/common/graph_topology_decoder.o \
  $(OUTPUT_PATH)/common/parser_types.o \
  $(OUTPUT_PATH)/common/ast.o \
  $(OUTPUT_PATH)/common/grammar_lexer.o \
  $(OUTPUT_PATH)/common/grammar_parser.o \
  $(OUTPUT_PATH)/common/instruction_lexer.o \
  $(OUTPUT_PATH)/common/instruction_parser.o \
  $(OUTPUT_PATH)/common/memory_mapped_datafile.o \
  $(OUTPUT_PATH)/libtorch/memory_mapped_dataset.o \
  $(OUTPUT_PATH)/libtorch/memory_mapped_dataloader.o

.PHONY: memory_mapped_storage_objects
memory_mapped_storage_objects:
	$(MAKE) -C $(IMPL_PATH)/piaabo utils files json_parsing
	$(MAKE) -C $(IMPL_PATH)/piaabo/parse/bnf all
	$(MAKE) -C $(IMPL_PATH)/piaabo/math all
	$(MAKE) -C $(IMPL_PATH)/ujcamei/source/registry/types all
	$(MAKE) -C $(IMPL_PATH)/ujcamei/source/contract all
	$(MAKE) -C $(IMPL_PATH)/ujcamei/source/retrieval/storage/memory_mapped all

$(eval $(call TEST_ONEFILE, test_ujcamei_graph_anchor_edge_dataset, test_ujcamei_graph_anchor_edge_dataset.cpp, \
  $(libtorch_a) $(MEMORY_MAPPED_SOURCE_OBJS) $(LDLIBS_torch)))

$(TEST_OUT)/test_ujcamei_graph_anchor_edge_dataset: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/test_ujcamei_graph_anchor_edge_dataset: memory_mapped_storage_objects

.PHONY: all
all: $(TEST_OUT)/test_ujcamei_graph_anchor_edge_dataset
	@$(LOG_SUCCESS)

.PHONY: run
run: memory_mapped_storage_objects run-test_ujcamei_graph_anchor_edge_dataset

.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/test_ujcamei_graph_anchor_edge_dataset
