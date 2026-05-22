ROOT_PATH := ../../../../..
include $(ROOT_PATH)/Makefile.config

HERE_PATH  := $(TESTS_PATH)/bench/wikimyei/config/graph_first_specs
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

TEST_DEFAULT_LDLIBS :=

GRAPH_FIRST_SPECS_OBJS := \
  $(OUTPUT_PATH)/common/core/utils.o \
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
  $(OUTPUT_PATH)/common/instruction_parser.o

.PHONY: graph_first_specs_objects
graph_first_specs_objects:
	$(MAKE) -C $(IMPL_PATH)/piaabo utils
	$(MAKE) -C $(IMPL_PATH)/piaabo/parse/bnf all
	$(MAKE) -C $(IMPL_PATH)/piaabo/math all
	$(MAKE) -C $(IMPL_PATH)/ujcamei/source/registry/types all
	$(MAKE) -C $(IMPL_PATH)/ujcamei/source/contract all

$(eval $(call TEST_ONEFILE, test_wikimyei_graph_first_specs, test_wikimyei_graph_first_specs.cpp, \
  $(libtorch_a) $(GRAPH_FIRST_SPECS_OBJS) $(LDLIBS_torch)))

$(TEST_OUT)/test_wikimyei_graph_first_specs: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/test_wikimyei_graph_first_specs: graph_first_specs_objects

.PHONY: all
all: $(TEST_OUT)/test_wikimyei_graph_first_specs
	@$(LOG_SUCCESS)

.PHONY: run
run: graph_first_specs_objects run-test_wikimyei_graph_first_specs

.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/test_wikimyei_graph_first_specs
