# =============================================================================
# cuwacunu/hero/Makefile — HERO runtime binaries
# =============================================================================
ROOT_PATH := ../..
include $(ROOT_PATH)/Makefile.config

HERE_PATH := $(ROOT_PATH)/cuwacunu/hero
# Runtime utility binaries should live under src/cuwacunu/build.
TEST_OUT := $(ROOT_PATH)/cuwacunu/build
ORDER_PREP := curl_openai_prep

.PHONY: curl_openai_prep
curl_openai_prep:
	+$(MAKE) -C $(IMPL_PATH)/piaabo/https_compat all
	+$(MAKE) lib-bundles

TEST_DEFAULT_LDLIBS :=
HERO_CONFIG_MCP_OBJS := \
	$(OUTPUT_PATH)/cuwacunu/hero/hero_config_mcp.o \
	$(OUTPUT_PATH)/cuwacunu/hero/hero_config_store.o \
	$(OUTPUT_PATH)/cuwacunu/hero/hero_config_commands.o

$(OUTPUT_PATH)/cuwacunu/hero/hero_config_mcp.o: INCLUDES_EXTRA += $(LIBCURL_INCLUDE_PATHS)
$(OUTPUT_PATH)/cuwacunu/hero/hero_config_store.o: INCLUDES_EXTRA += $(LIBCURL_INCLUDE_PATHS)
$(OUTPUT_PATH)/cuwacunu/hero/hero_config_commands.o: INCLUDES_EXTRA += $(LIBCURL_INCLUDE_PATHS)

$(OUTPUT_PATH)/cuwacunu/hero/hero_config_mcp.o: $(HERE_PATH)/hero_config_mcp.cpp
	$(CC_RULE)
$(OUTPUT_PATH)/cuwacunu/hero/hero_config_store.o: $(HERE_PATH)/hero_config_store.cpp
	$(CC_RULE)
$(OUTPUT_PATH)/cuwacunu/hero/hero_config_commands.o: $(HERE_PATH)/hero_config_commands.cpp
	$(CC_RULE)

$(TEST_OUT)/hero_config_mcp: $(HERO_CONFIG_MCP_OBJS) $(libcurl_a) | $(ORDER_PREP)
	$(Q)mkdir -p $(@D) $(DEPDIR) $(OBJTMP)
	$(Q)$(LOG_START)
	$(Q)$(LOCAL_CLEAN)
	$(Q)$(CXX_LINK) $(LDFLAGS_E) \
	      $(HERO_CONFIG_MCP_OBJS) \
	      $(LDLIBS_common) \
	      -Wl,--start-group $(libcurl_a) -Wl,--end-group \
	      $(LDLIBS_curl) $(LDLIBS_EXTRA) \
	      -o $@
	$(Q)$(LOG_END)

CONFIG_HERO_MCP_BIN := $(ROOT_PATH)/config/hero_config_mcp

.PHONY: all
all: $(TEST_OUT)/hero_config_mcp
	@$(LOG_SUCCESS)

.PHONY: run
run: $(TEST_OUT)/hero_config_mcp
	$(Q)$(RUN_MODE) $(TEST_OUT)/hero_config_mcp

.PHONY: install-hero-mcp
install-hero-mcp: $(TEST_OUT)/hero_config_mcp
	@cp -f $(TEST_OUT)/hero_config_mcp $(CONFIG_HERO_MCP_BIN)
	@chmod 700 $(CONFIG_HERO_MCP_BIN)
	@printf "\033[2;33m[\033[90mUTIL: \033[2;32mINSTALLED\033[2;33m]\033[0m \033[90m%s\033[0m\n" "$(CONFIG_HERO_MCP_BIN)"

.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/hero_config_mcp $(HERO_CONFIG_MCP_OBJS)
