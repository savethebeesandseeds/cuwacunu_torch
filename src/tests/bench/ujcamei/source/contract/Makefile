ROOT_PATH := ../../../../..
include $(ROOT_PATH)/Makefile.config

HERE_PATH  := $(TESTS_PATH)/bench/ujcamei/source/contract
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

TEST_DEFAULT_LDLIBS :=

SOURCE_CONTRACT_OBJS := \
  $(OUTPUT_PATH)/common/core/utils.o \
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

.PHONY: source_contract_objects
source_contract_objects:
	$(MAKE) -C $(IMPL_PATH)/piaabo utils
	$(MAKE) -C $(IMPL_PATH)/piaabo/parse/bnf all
	$(MAKE) -C $(IMPL_PATH)/ujcamei/source/contract all

$(eval $(call TEST_ONEFILE, test_ujcamei_source_contract, test_ujcamei_source_contract.cpp, \
  $(SOURCE_CONTRACT_OBJS)))

$(TEST_OUT)/test_ujcamei_source_contract: source_contract_objects

.PHONY: all
all: $(TEST_OUT)/test_ujcamei_source_contract
	@$(LOG_SUCCESS)

.PHONY: run
run: source_contract_objects run-test_ujcamei_source_contract

.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/test_ujcamei_source_contract
