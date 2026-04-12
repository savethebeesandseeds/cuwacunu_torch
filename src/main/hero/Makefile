# =============================================================================
# main/hero/Makefile — HERO runtime binaries
# =============================================================================
ROOT_PATH := ../..
include $(ROOT_PATH)/Makefile.config

HERE_PATH := $(ROOT_PATH)/main/hero
# Runtime utility binaries should live under .build/hero.
BIN_OUT  := $(HERO_OUT)

TEST_DEFAULT_LDLIBS :=
HERO_DLOG_OVERRIDES := -DLOG_DBG_FILE=stderr

HERO_HASHIMYEI_LINK := $(libssl_a) $(libcommon_a) $(LDLIBS_ssl)
$(eval $(call BIN_ONEFILE, hero_hashimyei_mcp, hero_hashimyei_mcp.cpp, $(HERO_HASHIMYEI_LINK)))
$(BIN_OUT)/hero_hashimyei_%: INCLUDES_EXTRA += $(HERO_DLOG_OVERRIDES) $(SSL_INCLUDE_PATHS)

HERO_LATTICE_LINK := $(libcurl_a) $(libssl_a) $(libcommon_a) $(LDLIBS_curl) $(LDLIBS_ssl)
$(eval $(call BIN_ONEFILE, hero_lattice_mcp, hero_lattice_mcp.cpp, $(HERO_LATTICE_LINK)))
$(BIN_OUT)/hero_lattice_mcp: INCLUDES_EXTRA += $(HERO_DLOG_OVERRIDES) $(SSL_INCLUDE_PATHS)

HERO_CONFIG_LINK := $(libcommon_a) $(libssl_a) $(LDLIBS_ssl)
$(eval $(call BIN_ONEFILE, hero_config_mcp, hero_config_mcp.cpp, $(HERO_CONFIG_LINK)))
$(BIN_OUT)/hero_config_mcp: INCLUDES_EXTRA += $(HERO_DLOG_OVERRIDES) $(SSL_INCLUDE_PATHS)

HERO_RUNTIME_LINK := $(libcommon_a) $(libssl_a) $(LDLIBS_ssl)
$(eval $(call BIN_ONEFILE, hero_runtime_mcp, hero_runtime_mcp.cpp, $(HERO_RUNTIME_LINK)))
$(BIN_OUT)/hero_runtime_mcp: INCLUDES_EXTRA += $(HERO_DLOG_OVERRIDES) $(SSL_INCLUDE_PATHS)

HERO_MARSHAL_LINK := $(libcommon_a) $(libssl_a) $(LDLIBS_ssl)
$(eval $(call BIN_ONEFILE, hero_marshal_mcp, hero_marshal_mcp.cpp, $(HERO_MARSHAL_LINK)))
$(BIN_OUT)/hero_marshal_mcp: INCLUDES_EXTRA += $(HERO_DLOG_OVERRIDES) $(SSL_INCLUDE_PATHS)

HERO_HUMAN_LINK := $(libcommon_a) $(libssl_a) $(LDLIBS_ssl) $(LDLIBS_ncurses)
$(eval $(call BIN_ONEFILE, hero_human_mcp, hero_human_mcp.cpp, $(HERO_HUMAN_LINK)))
$(BIN_OUT)/hero_human_mcp: INCLUDES_EXTRA += $(HERO_DLOG_OVERRIDES) $(SSL_INCLUDE_PATHS) $(NCURSES_INCLUDE_PATHS)

define INSTALL_HERO_BIN
.PHONY: install-$(strip $(1))
install-$(strip $(1)): $(BIN_OUT)/$(strip $(2))
	@mode="$$(stat -c '%a' "$(BIN_OUT)/$(strip $(2))" 2>/dev/null || echo missing)"; \
	if [ "$$mode" != "700" ]; then \
		chmod 700 "$(BIN_OUT)/$(strip $(2))"; \
		printf "\033[2;33m[\033[90mHERO: \033[2;32mINSTALLED\033[2;33m]\033[0m \033[90m%s\033[0m\n" "$(BIN_OUT)/$(strip $(2))"; \
	fi
endef

$(eval $(call INSTALL_HERO_BIN,config-hero,hero_config_mcp))
$(eval $(call INSTALL_HERO_BIN,hashimyei-hero,hero_hashimyei_mcp))
$(eval $(call INSTALL_HERO_BIN,lattice-hero,hero_lattice_mcp))
$(eval $(call INSTALL_HERO_BIN,runtime-hero,hero_runtime_mcp))
$(eval $(call INSTALL_HERO_BIN,marshal-hero,hero_marshal_mcp))
$(eval $(call INSTALL_HERO_BIN,human-hero,hero_human_mcp))

.PHONY: all
all: $(BIN_OUT)/hero_config_mcp $(BIN_OUT)/hero_hashimyei_mcp $(BIN_OUT)/hero_lattice_mcp $(BIN_OUT)/hero_runtime_mcp $(BIN_OUT)/hero_marshal_mcp $(BIN_OUT)/hero_human_mcp
	@$(LOG_SUCCESS)

.PHONY: run
run: $(BIN_OUT)/hero_config_mcp
	$(Q)$(RUN_MODE) $(BIN_OUT)/hero_config_mcp

.PHONY: clean
clean:
	$(Q)$(CLEAN_RM_CMD) $(BIN_OUT)/hero_config_mcp \
	  $(BIN_OUT)/hero_hashimyei_mcp \
	  $(BIN_OUT)/hero_lattice_mcp \
	  $(BIN_OUT)/hero_runtime_mcp \
	  $(BIN_OUT)/hero_marshal_mcp \
	  $(BIN_OUT)/hero_human_mcp
