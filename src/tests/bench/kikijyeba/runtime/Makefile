ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config

HERE_PATH  := $(TESTS_PATH)/bench/kikijyeba/runtime
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

TEST_DEFAULT_LDLIBS :=

KIKIJYEBA_RUNTIME_OBJS := \
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

$(OUTPUT_PATH)/common/hero_runtime_tools.o: \
	$(IMPL_PATH)/hero/runtime_hero/hero_runtime_tools.cpp \
	$(SRC_ROOT)/Makefile.config
	$(CC_RULE)

.PHONY: kikijyeba_job_runner_objects
kikijyeba_job_runner_objects:
	$(MAKE) -C $(IMPL_PATH)/piaabo utils files json_parsing
	$(MAKE) -C $(IMPL_PATH)/piaabo/parse/bnf all
	$(MAKE) -C $(IMPL_PATH)/piaabo/math all
	$(MAKE) -C $(IMPL_PATH)/ujcamei/source/registry/types all
	$(MAKE) -C $(IMPL_PATH)/ujcamei/source/contract all
	$(MAKE) -C $(IMPL_PATH)/ujcamei/source/retrieval/storage/memory_mapped all
	$(MAKE) -C $(IMPL_PATH)/wikimyei/inference/expected_value/mdn all

$(eval $(call TEST_ONEFILE, test_kikijyeba_job_runner, test_kikijyeba_job_runner.cpp, \
  $(libtorch_a) $(KIKIJYEBA_RUNTIME_OBJS) $(LDLIBS_torch)))

$(eval $(call TEST_ONEFILE, test_hero_runtime_wave_preview, test_hero_runtime_wave_preview.cpp, \
  $(OUTPUT_PATH)/common/hero_runtime_tools.o))

$(eval $(call TEST_ONEFILE, test_hero_mcp_schema_compat, test_hero_mcp_schema_compat.cpp))

$(TEST_OUT)/test_kikijyeba_job_runner: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/test_kikijyeba_job_runner: kikijyeba_job_runner_objects

.PHONY: hero_mcp_schema_catalogs
hero_mcp_schema_catalogs:
	$(MAKE) -C $(SRC_ROOT)/main/hero build-config-hero build-runtime-hero build-lattice-hero

$(TEST_OUT)/test_hero_mcp_schema_compat: hero_mcp_schema_catalogs

.PHONY: all
all: $(TEST_OUT)/test_kikijyeba_job_runner $(TEST_OUT)/test_hero_runtime_wave_preview $(TEST_OUT)/test_hero_mcp_schema_compat
	@$(LOG_SUCCESS)

.PHONY: run
run: kikijyeba_job_runner_objects run-test_kikijyeba_job_runner run-test_hero_runtime_wave_preview run-test_hero_mcp_schema_compat

.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/test_kikijyeba_job_runner $(TEST_OUT)/test_hero_runtime_wave_preview $(TEST_OUT)/test_hero_mcp_schema_compat
