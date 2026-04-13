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
$(eval $(call BIN_ONEFILE, hero_hashimyei.mcp, hero_hashimyei_mcp.cpp, $(HERO_HASHIMYEI_LINK)))
$(BIN_OUT)/hero_hashimyei.mcp: INCLUDES_EXTRA += $(HERO_DLOG_OVERRIDES) $(SSL_INCLUDE_PATHS)

HERO_LATTICE_LINK := $(libcurl_a) $(libssl_a) $(libcommon_a) $(LDLIBS_curl) $(LDLIBS_ssl)
$(eval $(call BIN_ONEFILE, hero_lattice.mcp, hero_lattice_mcp.cpp, $(HERO_LATTICE_LINK)))
$(BIN_OUT)/hero_lattice.mcp: INCLUDES_EXTRA += $(HERO_DLOG_OVERRIDES) $(SSL_INCLUDE_PATHS)

HERO_CONFIG_LINK := $(libcommon_a) $(libssl_a) $(LDLIBS_ssl)
$(eval $(call BIN_ONEFILE, hero_config.mcp, hero_config_mcp.cpp, $(HERO_CONFIG_LINK)))
$(BIN_OUT)/hero_config.mcp: INCLUDES_EXTRA += $(HERO_DLOG_OVERRIDES) $(SSL_INCLUDE_PATHS)

HERO_RUNTIME_LINK := $(libcommon_a) $(libssl_a) $(LDLIBS_ssl)
$(eval $(call BIN_ONEFILE, hero_runtime.mcp, hero_runtime_mcp.cpp, $(HERO_RUNTIME_LINK)))
$(BIN_OUT)/hero_runtime.mcp: INCLUDES_EXTRA += $(HERO_DLOG_OVERRIDES) $(SSL_INCLUDE_PATHS)

HERO_MARSHAL_LINK := $(libcommon_a) $(libssl_a) $(LDLIBS_ssl)
$(eval $(call BIN_ONEFILE, hero_marshal.mcp, hero_marshal_mcp.cpp, $(HERO_MARSHAL_LINK)))
$(BIN_OUT)/hero_marshal.mcp: INCLUDES_EXTRA += $(HERO_DLOG_OVERRIDES) $(SSL_INCLUDE_PATHS)

HERO_HUMAN_LINK := $(libcommon_a) $(libssl_a) $(LDLIBS_ssl) $(LDLIBS_ncurses)
$(eval $(call BIN_ONEFILE, hero_human.mcp, hero_human_mcp.cpp, $(HERO_HUMAN_LINK)))
$(BIN_OUT)/hero_human.mcp: INCLUDES_EXTRA += $(HERO_DLOG_OVERRIDES) $(SSL_INCLUDE_PATHS) $(NCURSES_INCLUDE_PATHS)

define INSTALL_HERO_BIN
.PHONY: install-$(strip $(1))
install-$(strip $(1)): $(BIN_OUT)/$(strip $(2))
	@mode="$$(stat -c '%a' "$(BIN_OUT)/$(strip $(2))" 2>/dev/null || echo missing)"; \
	if [ "$$mode" != "700" ]; then \
		chmod 700 "$(BIN_OUT)/$(strip $(2))"; \
		printf "\033[2;33m[\033[90mHERO: \033[2;32mINSTALLED\033[2;33m]\033[0m \033[90m%s\033[0m\n" "$(BIN_OUT)/$(strip $(2))"; \
	fi
endef

$(eval $(call INSTALL_HERO_BIN,config-hero,hero_config.mcp))
$(eval $(call INSTALL_HERO_BIN,hashimyei-hero,hero_hashimyei.mcp))
$(eval $(call INSTALL_HERO_BIN,lattice-hero,hero_lattice.mcp))
$(eval $(call INSTALL_HERO_BIN,runtime-hero,hero_runtime.mcp))
$(eval $(call INSTALL_HERO_BIN,marshal-hero,hero_marshal.mcp))
$(eval $(call INSTALL_HERO_BIN,human-hero,hero_human.mcp))

.PHONY: all
all: $(BIN_OUT)/hero_config.mcp $(BIN_OUT)/hero_hashimyei.mcp $(BIN_OUT)/hero_lattice.mcp $(BIN_OUT)/hero_runtime.mcp $(BIN_OUT)/hero_marshal.mcp $(BIN_OUT)/hero_human.mcp
	@$(LOG_SUCCESS)

.PHONY: run
run: $(BIN_OUT)/hero_config.mcp
	$(Q)$(RUN_MODE) $(BIN_OUT)/hero_config.mcp

.PHONY: clean
clean:
	$(Q)$(CLEAN_RM_CMD) $(BIN_OUT)/hero_config.mcp \
	  $(BIN_OUT)/hero_hashimyei.mcp \
	  $(BIN_OUT)/hero_lattice.mcp \
	  $(BIN_OUT)/hero_runtime.mcp \
	  $(BIN_OUT)/hero_marshal.mcp \
	  $(BIN_OUT)/hero_human.mcp \
	  $(BIN_OUT)/hero_config_mcp \
	  $(BIN_OUT)/hero_hashimyei_mcp \
	  $(BIN_OUT)/hero_lattice_mcp \
	  $(BIN_OUT)/hero_runtime_mcp \
	  $(BIN_OUT)/hero_marshal_mcp \
	  $(BIN_OUT)/hero_human_mcp
