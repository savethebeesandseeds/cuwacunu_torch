ROOT_PATH := ../..
include $(ROOT_PATH)/Makefile.config

HERE_PATH := $(ROOT_PATH)/main/exec
BIN_OUT := $(BUILD_ROOT)/exec

CUWACUNU_EXEC_OBJS := \
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
  $(OUTPUT_PATH)/libtorch/memory_mapped_dataloader.o \
  $(OUTPUT_PATH)/libtorch/synthetic_reference_lift.o \
  $(OUTPUT_PATH)/libtorch/stream/srl_graph_adapter.o \
  $(OUTPUT_PATH)/libtorch/mixture_density_network.o

.PHONY: cuwacunu-exec-objects
cuwacunu-exec-objects:
	$(MAKE) -C $(IMPL_PATH)/piaabo utils files json_parsing
	$(MAKE) -C $(IMPL_PATH)/piaabo/parse/bnf all
	$(MAKE) -C $(IMPL_PATH)/piaabo/math all
	$(MAKE) -C $(IMPL_PATH)/ujcamei/source/registry/types all
	$(MAKE) -C $(IMPL_PATH)/ujcamei/source/contract all
	$(MAKE) -C $(IMPL_PATH)/ujcamei/source/retrieval/storage/memory_mapped all
	$(MAKE) -C $(IMPL_PATH)/wikimyei/expression/nodelift/srl all
	$(MAKE) -C $(IMPL_PATH)/wikimyei/inference/expected_value/mdn all

$(OBJTMP)/cuwacunu_exec.o: \
	$(HERE_PATH)/cuwacunu_exec.cpp \
	$(HERE_PATH)/Makefile \
	$(SRC_ROOT)/Makefile.config
	$(Q)mkdir -p $(@D) $(DEPDIR)
	$(Q)$(LOG_START)
	$(Q)$(LOCAL_CLEAN)
	$(Q)$(CXX_COMPILE) $(CXXFLAGS) \
	      -MMD -MP -MF $(DEPDIR)/cuwacunu_exec.d -MT $@ \
	      $(USUAL_INCLUDE_PATHS) $(TORCH_INCLUDE_PATHS) \
	      -c $(abspath $<) -o $@
	$(Q)$(LOG_END)

$(BIN_OUT)/cuwacunu_exec: \
	$(OBJTMP)/cuwacunu_exec.o \
	$(CUWACUNU_EXEC_OBJS) \
	$(HERE_PATH)/Makefile \
	$(SRC_ROOT)/Makefile.config | cuwacunu-exec-objects
	$(Q)mkdir -p $(@D)
	$(Q)$(LOG_START)
	$(Q)$(LOCAL_CLEAN)
	$(Q)$(CXX_LINK) $(LDFLAGS_E) \
	      $(OBJTMP)/cuwacunu_exec.o \
	      $(CUWACUNU_EXEC_OBJS) \
	      $(LDLIBS_torch) $(LDLIBS_common) \
	      -o $@
	$(Q)$(LOG_END)

.PHONY: all build-cuwacunu-exec run-cuwacunu-exec install-cuwacunu-exec clean

all build-cuwacunu-exec: $(BIN_OUT)/cuwacunu_exec
	@$(LOG_SUCCESS)

run-cuwacunu-exec: $(BIN_OUT)/cuwacunu_exec
	$(Q)$(RUN_MODE) $(BIN_OUT)/cuwacunu_exec --help

install-cuwacunu-exec: $(BIN_OUT)/cuwacunu_exec
	@mode="$$(stat -c '%a' "$(BIN_OUT)/cuwacunu_exec" 2>/dev/null || echo missing)"; \
	if [ "$$mode" != "700" ]; then \
		chmod 700 "$(BIN_OUT)/cuwacunu_exec"; \
		printf "\033[2;33m[\033[90mEXEC: \033[2;32mINSTALLED\033[2;33m]\033[0m \033[90m%s\033[0m\n" "$(BIN_OUT)/cuwacunu_exec"; \
	fi

clean:
	$(Q)$(CLEAN_RM_CMD) $(BIN_OUT)/cuwacunu_exec $(OBJTMP)/cuwacunu_exec.o
