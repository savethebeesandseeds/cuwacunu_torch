# =============================================================================
# main/hero/Makefile — HERO runtime binaries
# =============================================================================
ROOT_PATH := ../..
include $(ROOT_PATH)/Makefile.config

HERE_PATH := $(ROOT_PATH)/main/hero
CONFIG_HERO_PATH := $(HERE_PATH)/config_hero
# Runtime utility binaries should live under .build/hero.
TEST_OUT := $(HERO_OUT)

TEST_DEFAULT_LDLIBS :=
HERO_BOARD_LINK := $(libtorch_a) $(libcurl_a) $(libssl_a) $(libcommon_a) $(LDLIBS_torch) $(LDLIBS_curl) $(LDLIBS_ssl)
$(eval $(call TEST_ONEFILE, hero_board, board_hero/hero_board.cpp, $(HERO_BOARD_LINK)))
$(TEST_OUT)/hero_board: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS)

HERO_HASHIMYEI_LINK := $(libtorch_a) $(libssl_a) $(libcommon_a) $(LDLIBS_torch) $(LDLIBS_ssl)
$(eval $(call TEST_ONEFILE, hero_hashimyei_ingest, hashimyei_hero/hero_hashimyei_ingest.cpp, $(HERO_HASHIMYEI_LINK)))
$(eval $(call TEST_ONEFILE, hero_hashimyei_mcp, hashimyei_hero/hero_hashimyei_mcp.cpp, $(HERO_HASHIMYEI_LINK)))
$(TEST_OUT)/hero_hashimyei_%: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS)

HERO_CONFIG_MCP_OBJS := \
	$(OUTPUT_PATH)/main/hero/config_hero/hero_config_mcp.o \
	$(OUTPUT_PATH)/main/hero/config_hero/hero_config_store.o \
	$(OUTPUT_PATH)/main/hero/config_hero/hero_config_commands.o

$(OUTPUT_PATH)/main/hero/config_hero/hero_config_mcp.o: $(CONFIG_HERO_PATH)/hero_config_mcp.cpp
	$(CC_RULE)
$(OUTPUT_PATH)/main/hero/config_hero/hero_config_store.o: $(CONFIG_HERO_PATH)/hero_config_store.cpp
	$(CC_RULE)
$(OUTPUT_PATH)/main/hero/config_hero/hero_config_commands.o: $(CONFIG_HERO_PATH)/hero_config_commands.cpp
	$(CC_RULE)

$(TEST_OUT)/hero_config_mcp: $(HERO_CONFIG_MCP_OBJS)
	$(Q)mkdir -p $(@D) $(DEPDIR) $(OBJTMP)
	$(Q)$(LOG_START)
	$(Q)$(LOCAL_CLEAN)
	$(Q)$(CXX_LINK) $(LDFLAGS_E) \
	      $(HERO_CONFIG_MCP_OBJS) \
	      $(LDLIBS_common) \
	      $(LDLIBS_EXTRA) \
	      -o $@
	$(Q)$(LOG_END)

CONFIG_HERO_MCP_BIN := $(ROOT_PATH)/config/hero_config_mcp
HASHIMYEI_INGEST_BIN := $(ROOT_PATH)/config/hero_hashimyei_ingest
HASHIMYEI_MCP_BIN := $(ROOT_PATH)/config/hero_hashimyei_mcp

.PHONY: all
all: $(TEST_OUT)/hero_config_mcp $(TEST_OUT)/hero_hashimyei_ingest $(TEST_OUT)/hero_hashimyei_mcp
	@$(LOG_SUCCESS)

.PHONY: run
run: $(TEST_OUT)/hero_config_mcp
	$(Q)$(RUN_MODE) $(TEST_OUT)/hero_config_mcp

.PHONY: build-board_hero
build-board_hero: $(TEST_OUT)/hero_board
	@$(LOG_SUCCESS)

.PHONY: run-board_hero
run-board_hero: $(TEST_OUT)/hero_board
	$(Q)$(RUN_MODE) $(TEST_OUT)/hero_board

.PHONY: board_hero
board_hero: run-board_hero

.PHONY: install-hero-mcp
install-hero-mcp: $(TEST_OUT)/hero_config_mcp
	@cp -f $(TEST_OUT)/hero_config_mcp $(CONFIG_HERO_MCP_BIN)
	@chmod 700 $(CONFIG_HERO_MCP_BIN)
	@printf "\033[2;33m[\033[90mUTIL: \033[2;32mINSTALLED\033[2;33m]\033[0m \033[90m%s\033[0m\n" "$(CONFIG_HERO_MCP_BIN)"

.PHONY: install-hashimyei-tools
install-hashimyei-tools: $(TEST_OUT)/hero_hashimyei_ingest $(TEST_OUT)/hero_hashimyei_mcp
	@cp -f $(TEST_OUT)/hero_hashimyei_ingest $(HASHIMYEI_INGEST_BIN)
	@cp -f $(TEST_OUT)/hero_hashimyei_mcp $(HASHIMYEI_MCP_BIN)
	@chmod 700 $(HASHIMYEI_INGEST_BIN) $(HASHIMYEI_MCP_BIN)
	@printf "\033[2;33m[\033[90mUTIL: \033[2;32mINSTALLED\033[2;33m]\033[0m \033[90m%s\033[0m\n" "$(HASHIMYEI_INGEST_BIN)"
	@printf "\033[2;33m[\033[90mUTIL: \033[2;32mINSTALLED\033[2;33m]\033[0m \033[90m%s\033[0m\n" "$(HASHIMYEI_MCP_BIN)"

.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/hero_config_mcp $(TEST_OUT)/hero_board \
	  $(TEST_OUT)/hero_hashimyei_ingest $(TEST_OUT)/hero_hashimyei_mcp \
	  $(HERO_CONFIG_MCP_OBJS)
