ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config

HERE_PATH  := $(TESTS_PATH)/bench/piaabo
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

TEST_DEFAULT_LDLIBS :=

PIAABO_PARSE_IO_OBJS := \
  $(OUTPUT_PATH)/common/core/utils.o \
  $(OUTPUT_PATH)/common/io/files.o \
  $(OUTPUT_PATH)/common/parse/json/json_parsing.o \
  $(OUTPUT_PATH)/common/parser_types.o \
  $(OUTPUT_PATH)/common/ast.o \
  $(OUTPUT_PATH)/common/grammar_lexer.o \
  $(OUTPUT_PATH)/common/grammar_parser.o \
  $(OUTPUT_PATH)/common/instruction_lexer.o \
  $(OUTPUT_PATH)/common/instruction_parser.o

PIAABO_TORCH_DISTRIBUTION_OBJS := \
  $(OUTPUT_PATH)/common/core/utils.o \
  $(OUTPUT_PATH)/libtorch/torch_utils.o \
  $(OUTPUT_PATH)/libtorch/gamma.o \
  $(OUTPUT_PATH)/libtorch/beta.o \
  $(OUTPUT_PATH)/libtorch/categorical.o

.PHONY: piaabo_parse_io_objects
piaabo_parse_io_objects:
	$(MAKE) -C $(IMPL_PATH)/piaabo utils files json_parsing
	$(MAKE) -C $(IMPL_PATH)/piaabo/parse/bnf all

.PHONY: piaabo_torch_distribution_objects
piaabo_torch_distribution_objects:
	$(MAKE) -C $(IMPL_PATH)/piaabo utils
	$(MAKE) -C $(IMPL_PATH)/piaabo/tensor/torch torch_utils
	$(MAKE) -C $(IMPL_PATH)/piaabo/tensor/torch/distributions all

$(eval $(call TEST_ONEFILE, test_piaabo_parse_io_contracts, test_piaabo_parse_io_contracts.cpp, \
  $(PIAABO_PARSE_IO_OBJS)))

$(eval $(call TEST_ONEFILE, test_piaabo_torch_distributions, test_piaabo_torch_distributions.cpp, \
  $(PIAABO_TORCH_DISTRIBUTION_OBJS) $(LDLIBS_torch)))

$(TEST_OUT)/test_piaabo_parse_io_contracts: piaabo_parse_io_objects
$(TEST_OUT)/test_piaabo_torch_distributions: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/test_piaabo_torch_distributions: piaabo_torch_distribution_objects

.PHONY: all
all: $(TEST_OUT)/test_piaabo_parse_io_contracts $(TEST_OUT)/test_piaabo_torch_distributions
	@$(LOG_SUCCESS)

.PHONY: run
run: piaabo_parse_io_objects piaabo_torch_distribution_objects \
     run-test_piaabo_parse_io_contracts run-test_piaabo_torch_distributions

.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/test_piaabo_parse_io_contracts
	@rm -f $(TEST_OUT)/test_piaabo_torch_distributions
