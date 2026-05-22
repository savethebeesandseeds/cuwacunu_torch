ROOT_PATH := ../..
include $(ROOT_PATH)/Makefile.config

HERE_PATH := $(ROOT_PATH)/main/hero
BIN_OUT := $(HERO_OUT)

HERO_CONFIG_OBJS := \
  $(OUTPUT_PATH)/common/hero_config_store.o \
  $(OUTPUT_PATH)/common/hero_config_tools.o

HERO_RUNTIME_OBJS := \
  $(OUTPUT_PATH)/common/hero_runtime_tools.o

HERO_LATTICE_OBJS := \
  $(OUTPUT_PATH)/common/hero_lattice_tools.o

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

$(OUTPUT_PATH)/common/hero_lattice_tools.o: \
	$(IMPL_PATH)/hero/lattice_hero/hero_lattice_tools.cpp \
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

.PHONY: all build-config-hero build-runtime-hero build-lattice-hero run-config-hero run-runtime-hero run-lattice-hero install-config-hero install-runtime-hero install-lattice-hero clean

all: $(BIN_OUT)/hero_config.mcp $(BIN_OUT)/hero_runtime.mcp $(BIN_OUT)/hero_lattice.mcp
	@$(LOG_SUCCESS)

build-config-hero: $(BIN_OUT)/hero_config.mcp
	@$(LOG_SUCCESS)

build-runtime-hero: $(BIN_OUT)/hero_runtime.mcp
	@$(LOG_SUCCESS)

build-lattice-hero: $(BIN_OUT)/hero_lattice.mcp
	@$(LOG_SUCCESS)

run-config-hero: $(BIN_OUT)/hero_config.mcp
	$(Q)$(RUN_MODE) $(BIN_OUT)/hero_config.mcp --tool hero.config.status --args-json '{}'

run-runtime-hero: $(BIN_OUT)/hero_runtime.mcp
	$(Q)$(RUN_MODE) $(BIN_OUT)/hero_runtime.mcp --tool hero.runtime.status --args-json '{}'

run-lattice-hero: $(BIN_OUT)/hero_lattice.mcp
	$(Q)$(RUN_MODE) $(BIN_OUT)/hero_lattice.mcp --tool hero.lattice.status --args-json '{}'

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

install-lattice-hero: $(BIN_OUT)/hero_lattice.mcp
	@mode="$$(stat -c '%a' "$(BIN_OUT)/hero_lattice.mcp" 2>/dev/null || echo missing)"; \
	if [ "$$mode" != "700" ]; then \
		chmod 700 "$(BIN_OUT)/hero_lattice.mcp"; \
		printf "\033[2;33m[\033[90mHERO: \033[2;32mINSTALLED\033[2;33m]\033[0m \033[90m%s\033[0m\n" "$(BIN_OUT)/hero_lattice.mcp"; \
	fi

clean:
	$(Q)$(CLEAN_RM_CMD) $(BIN_OUT)/hero_config.mcp $(BIN_OUT)/hero_runtime.mcp $(BIN_OUT)/hero_lattice.mcp $(OBJTMP)/hero_config_mcp.o $(OBJTMP)/hero_runtime_mcp.o $(OBJTMP)/hero_lattice_mcp.o
