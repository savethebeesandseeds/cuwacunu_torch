ROOT_PATH := ../..
include $(ROOT_PATH)/Makefile.config

HERE_PATH := $(ROOT_PATH)/main/hero
BIN_OUT := $(HERO_OUT)

HERO_CONFIG_OBJS := \
  $(OUTPUT_PATH)/common/hero_config_store.o \
  $(OUTPUT_PATH)/common/hero_config_tools.o

HERO_RUNTIME_OBJS := \
  $(OUTPUT_PATH)/common/hero_runtime_tools.o \
  $(OUTPUT_PATH)/common/source_contract.o \
  $(OUTPUT_PATH)/common/source_contract_decode.o \
  $(OUTPUT_PATH)/common/source_registry_decoder.o \
  $(OUTPUT_PATH)/common/retrieval_channel_decoder.o \
  $(OUTPUT_PATH)/common/graph_topology_decoder.o \
  $(OUTPUT_PATH)/common/core/utils.o \
  $(OUTPUT_PATH)/common/io/files.o \
  $(OUTPUT_PATH)/common/parser_types.o \
  $(OUTPUT_PATH)/common/ast.o \
  $(OUTPUT_PATH)/common/grammar_lexer.o \
  $(OUTPUT_PATH)/common/grammar_parser.o \
  $(OUTPUT_PATH)/common/instruction_lexer.o \
  $(OUTPUT_PATH)/common/instruction_parser.o

HERO_ENVIRONMENT_TOOL_OBJ := \
  $(OUTPUT_PATH)/common/hero_environment_tools.o

HERO_ENVIRONMENT_OBJS := \
  $(HERO_ENVIRONMENT_TOOL_OBJ)

HERO_LATTICE_OBJS := \
  $(OUTPUT_PATH)/common/hero_lattice_tools.o \
  $(OUTPUT_PATH)/common/runtime_lls.o \
  $(OUTPUT_PATH)/common/lattice.o \
  $(OUTPUT_PATH)/common/core/utils.o \
  $(OUTPUT_PATH)/common/io/files.o \
  $(OUTPUT_PATH)/common/parser_types.o \
  $(OUTPUT_PATH)/common/ast.o \
  $(OUTPUT_PATH)/common/grammar_lexer.o \
  $(OUTPUT_PATH)/common/grammar_parser.o \
  $(OUTPUT_PATH)/common/instruction_lexer.o \
  $(OUTPUT_PATH)/common/instruction_parser.o

HERO_MARSHAL_OBJS := \
  $(HERO_LATTICE_OBJS)

$(OUTPUT_PATH)/common/hero_config_store.o: \
	$(IMPL_PATH)/hero/config_hero/hero_config_store.cpp \
	$(SRC_ROOT)/Makefile.config
	$(CC_RULE)

$(OUTPUT_PATH)/common/hero_config_tools.o: INCLUDES_EXTRA += $(SSL_INCLUDE_PATHS)
$(OUTPUT_PATH)/common/hero_config_tools.o: \
	$(IMPL_PATH)/hero/config_hero/hero_config_tools.cpp \
	$(SRC_ROOT)/Makefile.config
	$(CC_RULE)

$(OUTPUT_PATH)/common/hero_runtime_tools.o: \
	$(IMPL_PATH)/hero/runtime_hero/hero_runtime_tools.cpp \
	$(SRC_ROOT)/Makefile.config
	$(CC_RULE)
$(OUTPUT_PATH)/common/hero_runtime_tools.o: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)

$(OUTPUT_PATH)/common/source_contract.o: \
	$(IMPL_PATH)/ujcamei/source/contract/contract.cpp \
	$(SRC_ROOT)/Makefile.config
	$(CC_RULE)

$(OUTPUT_PATH)/common/source_contract_decode.o: \
	$(IMPL_PATH)/ujcamei/source/contract/runtime/decode.cpp \
	$(SRC_ROOT)/Makefile.config
	$(CC_RULE)

$(OUTPUT_PATH)/common/source_registry_decoder.o: \
	$(IMPL_PATH)/ujcamei/source/registry/source_registry_decoder.cpp \
	$(SRC_ROOT)/Makefile.config
	$(CC_RULE)

$(OUTPUT_PATH)/common/retrieval_channel_decoder.o: \
	$(IMPL_PATH)/ujcamei/source/retrieval/retrieval_channel_decoder.cpp \
	$(SRC_ROOT)/Makefile.config
	$(CC_RULE)

$(OUTPUT_PATH)/common/graph_topology_decoder.o: \
	$(IMPL_PATH)/kikijyeba/topology/graph/graph_topology_decoder.cpp \
	$(SRC_ROOT)/Makefile.config
	$(CC_RULE)

$(OUTPUT_PATH)/common/hero_environment_tools.o: \
	$(IMPL_PATH)/hero/environment_hero/hero_environment_tools.cpp \
	$(SRC_ROOT)/Makefile.config
	$(CC_RULE)

$(OUTPUT_PATH)/common/hero_lattice_tools.o: \
	$(IMPL_PATH)/hero/lattice_hero/hero_lattice_tools.cpp \
	$(SRC_ROOT)/Makefile.config
	$(CC_RULE)

$(OUTPUT_PATH)/common/runtime_lls.o: \
	$(IMPL_PATH)/hero/lattice_hero/lattice/runtime_report/runtime_lls.cpp \
	$(SRC_ROOT)/Makefile.config
	$(CC_RULE)

$(OUTPUT_PATH)/common/lattice.o: \
	$(IMPL_PATH)/hero/lattice_hero/lattice/lattice.cpp \
	$(SRC_ROOT)/Makefile.config
	$(CC_RULE)

$(OUTPUT_PATH)/common/core/utils.o: \
	$(IMPL_PATH)/piaabo/core/utils.cpp \
	$(SRC_ROOT)/Makefile.config
	$(CC_RULE)

$(OUTPUT_PATH)/common/io/files.o: \
	$(IMPL_PATH)/piaabo/io/files.cpp \
	$(SRC_ROOT)/Makefile.config
	$(CC_RULE)

$(OUTPUT_PATH)/common/parser_types.o: \
	$(IMPL_PATH)/piaabo/parse/bnf/parser_types.cpp \
	$(SRC_ROOT)/Makefile.config
	$(CC_RULE)

$(OUTPUT_PATH)/common/ast.o: \
	$(IMPL_PATH)/piaabo/parse/bnf/ast.cpp \
	$(SRC_ROOT)/Makefile.config
	$(CC_RULE)

$(OUTPUT_PATH)/common/grammar_lexer.o: \
	$(IMPL_PATH)/piaabo/parse/bnf/grammar_lexer.cpp \
	$(SRC_ROOT)/Makefile.config
	$(CC_RULE)

$(OUTPUT_PATH)/common/grammar_parser.o: \
	$(IMPL_PATH)/piaabo/parse/bnf/grammar_parser.cpp \
	$(SRC_ROOT)/Makefile.config
	$(CC_RULE)

$(OUTPUT_PATH)/common/instruction_lexer.o: \
	$(IMPL_PATH)/piaabo/parse/bnf/instruction_lexer.cpp \
	$(SRC_ROOT)/Makefile.config
	$(CC_RULE)

$(OUTPUT_PATH)/common/instruction_parser.o: \
	$(IMPL_PATH)/piaabo/parse/bnf/instruction_parser.cpp \
	$(SRC_ROOT)/Makefile.config
	$(CC_RULE)

$(OBJTMP)/hero_config_mcp.o: \
	$(HERE_PATH)/hero_config_mcp.cpp \
	$(HERE_PATH)/Makefile \
	$(SRC_ROOT)/Makefile.config
	$(Q)mkdir -p $(@D) $(DEPDIR)
	$(Q)$(LOG_START)
	$(Q)$(LOCAL_CLEAN)
	$(Q)$(CXX_COMPILE) $(CXXFLAGS) \
	      -MMD -MP -MF $(DEPDIR)/hero_config_mcp.d -MT $@ \
	      $(USUAL_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS) \
	      -c $(abspath $<) -o $@
	$(Q)$(LOG_END)

$(OBJTMP)/hero_runtime_mcp.o: \
	$(HERE_PATH)/hero_runtime_mcp.cpp \
	$(HERE_PATH)/Makefile \
	$(SRC_ROOT)/Makefile.config
	$(Q)mkdir -p $(@D) $(DEPDIR)
	$(Q)$(LOG_START)
	$(Q)$(LOCAL_CLEAN)
	$(Q)$(CXX_COMPILE) $(CXXFLAGS) \
	      -MMD -MP -MF $(DEPDIR)/hero_runtime_mcp.d -MT $@ \
	      $(USUAL_INCLUDE_PATHS) \
	      -c $(abspath $<) -o $@
	$(Q)$(LOG_END)

$(OBJTMP)/hero_environment_mcp.o: \
	$(HERE_PATH)/hero_environment_mcp.cpp \
	$(HERE_PATH)/Makefile \
	$(SRC_ROOT)/Makefile.config
	$(Q)mkdir -p $(@D) $(DEPDIR)
	$(Q)$(LOG_START)
	$(Q)$(LOCAL_CLEAN)
	$(Q)$(CXX_COMPILE) $(CXXFLAGS) \
	      -MMD -MP -MF $(DEPDIR)/hero_environment_mcp.d -MT $@ \
	      $(USUAL_INCLUDE_PATHS) \
	      -c $(abspath $<) -o $@
	$(Q)$(LOG_END)

$(OBJTMP)/hero_lattice_mcp.o: \
	$(HERE_PATH)/hero_lattice_mcp.cpp \
	$(HERE_PATH)/Makefile \
	$(SRC_ROOT)/Makefile.config
	$(Q)mkdir -p $(@D) $(DEPDIR)
	$(Q)$(LOG_START)
	$(Q)$(LOCAL_CLEAN)
	$(Q)$(CXX_COMPILE) $(CXXFLAGS) \
	      -MMD -MP -MF $(DEPDIR)/hero_lattice_mcp.d -MT $@ \
	      $(USUAL_INCLUDE_PATHS) \
	      -c $(abspath $<) -o $@
	$(Q)$(LOG_END)

$(OBJTMP)/hero_marshal_mcp.o: \
	$(HERE_PATH)/hero_marshal_mcp.cpp \
	$(HERE_PATH)/Makefile \
	$(SRC_ROOT)/Makefile.config
	$(Q)mkdir -p $(@D) $(DEPDIR)
	$(Q)$(LOG_START)
	$(Q)$(LOCAL_CLEAN)
	$(Q)$(CXX_COMPILE) $(CXXFLAGS) \
	      -MMD -MP -MF $(DEPDIR)/hero_marshal_mcp.d -MT $@ \
	      $(USUAL_INCLUDE_PATHS) \
	      -c $(abspath $<) -o $@
	$(Q)$(LOG_END)

$(BIN_OUT)/hero_config.mcp: \
	$(OBJTMP)/hero_config_mcp.o \
	$(HERO_CONFIG_OBJS) \
	$(HERE_PATH)/Makefile \
	$(SRC_ROOT)/Makefile.config
	$(Q)mkdir -p $(@D)
	$(Q)$(LOG_START)
	$(Q)$(LOCAL_CLEAN)
	$(Q)$(CXX_LINK) $(LDFLAGS_E) \
	      $(OBJTMP)/hero_config_mcp.o \
	      $(HERO_CONFIG_OBJS) \
	      $(LDLIBS_common) $(LDLIBS_ssl) \
	      -o $@
	$(Q)$(LOG_END)

$(BIN_OUT)/hero_runtime.mcp: \
	$(OBJTMP)/hero_runtime_mcp.o \
	$(HERO_RUNTIME_OBJS) \
	$(HERE_PATH)/Makefile \
	$(SRC_ROOT)/Makefile.config
	$(Q)mkdir -p $(@D)
	$(Q)$(LOG_START)
	$(Q)$(LOCAL_CLEAN)
	$(Q)$(CXX_LINK) $(LDFLAGS_E) \
	      $(OBJTMP)/hero_runtime_mcp.o \
	      $(HERO_RUNTIME_OBJS) \
	      $(LDLIBS_common) $(LDLIBS_torch) \
	      -o $@
	$(Q)$(LOG_END)

$(BIN_OUT)/hero_environment.mcp: \
	$(OBJTMP)/hero_environment_mcp.o \
	$(HERO_ENVIRONMENT_OBJS) \
	$(HERE_PATH)/Makefile \
	$(SRC_ROOT)/Makefile.config
	$(Q)mkdir -p $(@D)
	$(Q)$(LOG_START)
	$(Q)$(LOCAL_CLEAN)
	$(Q)$(CXX_LINK) $(LDFLAGS_E) \
	      $(OBJTMP)/hero_environment_mcp.o \
	      $(HERO_ENVIRONMENT_OBJS) \
	      $(LDLIBS_common) \
	      -o $@
	$(Q)$(LOG_END)

$(BIN_OUT)/hero_lattice.mcp: \
	$(OBJTMP)/hero_lattice_mcp.o \
	$(HERO_LATTICE_OBJS) \
	$(HERE_PATH)/Makefile \
	$(SRC_ROOT)/Makefile.config
	$(Q)mkdir -p $(@D)
	$(Q)$(LOG_START)
	$(Q)$(LOCAL_CLEAN)
	$(Q)$(CXX_LINK) $(LDFLAGS_E) \
	      $(OBJTMP)/hero_lattice_mcp.o \
	      $(HERO_LATTICE_OBJS) \
	      $(LDLIBS_common) \
	      -o $@
	$(Q)$(LOG_END)

$(BIN_OUT)/hero_marshal.mcp: \
	$(OBJTMP)/hero_marshal_mcp.o \
	$(HERO_MARSHAL_OBJS) \
	$(HERE_PATH)/Makefile \
	$(SRC_ROOT)/Makefile.config
	$(Q)mkdir -p $(@D)
	$(Q)$(LOG_START)
	$(Q)$(LOCAL_CLEAN)
	$(Q)$(CXX_LINK) $(LDFLAGS_E) \
	      $(OBJTMP)/hero_marshal_mcp.o \
	      $(HERO_MARSHAL_OBJS) \
	      $(LDLIBS_common) \
	      -o $@
	$(Q)$(LOG_END)

.PHONY: all build-config-hero build-runtime-hero build-environment-hero build-lattice-hero build-marshal-hero run-config-hero run-runtime-hero run-environment-hero run-lattice-hero run-marshal-hero install-config-hero install-runtime-hero install-environment-hero install-lattice-hero install-marshal-hero clean

all: $(BIN_OUT)/hero_config.mcp $(BIN_OUT)/hero_runtime.mcp $(BIN_OUT)/hero_environment.mcp $(BIN_OUT)/hero_lattice.mcp $(BIN_OUT)/hero_marshal.mcp
	@$(LOG_SUCCESS)

build-config-hero: $(BIN_OUT)/hero_config.mcp
	@$(LOG_SUCCESS)

build-runtime-hero: $(BIN_OUT)/hero_runtime.mcp
	@$(LOG_SUCCESS)

build-environment-hero: $(BIN_OUT)/hero_environment.mcp
	@$(LOG_SUCCESS)

build-lattice-hero: $(BIN_OUT)/hero_lattice.mcp
	@$(LOG_SUCCESS)

build-marshal-hero: $(BIN_OUT)/hero_marshal.mcp
	@$(LOG_SUCCESS)

run-config-hero: $(BIN_OUT)/hero_config.mcp
	$(Q)$(RUN_MODE) $(BIN_OUT)/hero_config.mcp --tool hero.config.status --args-json '{}'

run-runtime-hero: $(BIN_OUT)/hero_runtime.mcp
	$(Q)$(RUN_MODE) $(BIN_OUT)/hero_runtime.mcp --tool hero.runtime.status --args-json '{}'

run-environment-hero: $(BIN_OUT)/hero_environment.mcp
	$(Q)$(RUN_MODE) $(BIN_OUT)/hero_environment.mcp --tool hero.environment.status --args-json '{}'

run-lattice-hero: $(BIN_OUT)/hero_lattice.mcp
	$(Q)$(RUN_MODE) $(BIN_OUT)/hero_lattice.mcp --tool hero.lattice.status --args-json '{}'

run-marshal-hero: $(BIN_OUT)/hero_marshal.mcp
	$(Q)$(RUN_MODE) $(BIN_OUT)/hero_marshal.mcp --list-tools-json

install-config-hero: $(BIN_OUT)/hero_config.mcp
	@mode="$$(stat -c '%a' "$(BIN_OUT)/hero_config.mcp" 2>/dev/null || echo missing)"; \
	if [ "$$mode" != "700" ]; then \
		chmod 700 "$(BIN_OUT)/hero_config.mcp"; \
		printf "\033[2;33m[\033[90mHERO: \033[2;32mINSTALLED\033[2;33m]\033[0m \033[90m%s\033[0m\n" "$(BIN_OUT)/hero_config.mcp"; \
	fi

install-runtime-hero: $(BIN_OUT)/hero_runtime.mcp
	@mode="$$(stat -c '%a' "$(BIN_OUT)/hero_runtime.mcp" 2>/dev/null || echo missing)"; \
	if [ "$$mode" != "700" ]; then \
		chmod 700 "$(BIN_OUT)/hero_runtime.mcp"; \
		printf "\033[2;33m[\033[90mHERO: \033[2;32mINSTALLED\033[2;33m]\033[0m \033[90m%s\033[0m\n" "$(BIN_OUT)/hero_runtime.mcp"; \
	fi

install-environment-hero: $(BIN_OUT)/hero_environment.mcp
	@mode="$$(stat -c '%a' "$(BIN_OUT)/hero_environment.mcp" 2>/dev/null || echo missing)"; \
	if [ "$$mode" != "700" ]; then \
		chmod 700 "$(BIN_OUT)/hero_environment.mcp"; \
		printf "\033[2;33m[\033[90mHERO: \033[2;32mINSTALLED\033[2;33m]\033[0m \033[90m%s\033[0m\n" "$(BIN_OUT)/hero_environment.mcp"; \
	fi

install-lattice-hero: $(BIN_OUT)/hero_lattice.mcp
	@mode="$$(stat -c '%a' "$(BIN_OUT)/hero_lattice.mcp" 2>/dev/null || echo missing)"; \
	if [ "$$mode" != "700" ]; then \
		chmod 700 "$(BIN_OUT)/hero_lattice.mcp"; \
		printf "\033[2;33m[\033[90mHERO: \033[2;32mINSTALLED\033[2;33m]\033[0m \033[90m%s\033[0m\n" "$(BIN_OUT)/hero_lattice.mcp"; \
	fi

install-marshal-hero: $(BIN_OUT)/hero_marshal.mcp
	@mode="$$(stat -c '%a' "$(BIN_OUT)/hero_marshal.mcp" 2>/dev/null || echo missing)"; \
	if [ "$$mode" != "700" ]; then \
		chmod 700 "$(BIN_OUT)/hero_marshal.mcp"; \
		printf "\033[2;33m[\033[90mHERO: \033[2;32mINSTALLED\033[2;33m]\033[0m \033[90m%s\033[0m\n" "$(BIN_OUT)/hero_marshal.mcp"; \
	fi

clean:
	$(Q)$(CLEAN_RM_CMD) $(BIN_OUT)/hero_config.mcp $(BIN_OUT)/hero_runtime.mcp $(BIN_OUT)/hero_environment.mcp $(BIN_OUT)/hero_lattice.mcp $(BIN_OUT)/hero_marshal.mcp $(OBJTMP)/hero_config_mcp.o $(OBJTMP)/hero_runtime_mcp.o $(OBJTMP)/hero_environment_mcp.o $(OBJTMP)/hero_lattice_mcp.o $(OBJTMP)/hero_marshal_mcp.o
