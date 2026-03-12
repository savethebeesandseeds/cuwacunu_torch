# =============================================================================
# main/hero/Makefile — HERO runtime binaries
# =============================================================================
ROOT_PATH := ../..
include $(ROOT_PATH)/Makefile.config

HERE_PATH := $(ROOT_PATH)/main/hero
# Runtime utility binaries should live under .build/hero.
TEST_OUT := $(HERO_OUT)

TEST_DEFAULT_LDLIBS :=

HERO_HASHIMYEI_LINK := $(libtorch_a) $(libssl_a) $(libcommon_a) $(LDLIBS_torch) $(LDLIBS_ssl)
$(eval $(call TEST_ONEFILE, hero_hashimyei_mcp, hero_hashimyei_mcp.cpp, $(HERO_HASHIMYEI_LINK)))
$(TEST_OUT)/hero_hashimyei_%: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS)

HERO_LATTICE_LINK := $(libtorch_a) $(libcurl_a) $(libssl_a) $(libcommon_a) $(LDLIBS_torch) $(LDLIBS_curl) $(LDLIBS_ssl)
$(eval $(call TEST_ONEFILE, hero_lattice_mcp, hero_lattice_mcp.cpp, $(HERO_LATTICE_LINK)))
$(TEST_OUT)/hero_lattice_mcp: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS)

HERO_CONFIG_LINK := $(libcommon_a) $(libssl_a) $(LDLIBS_ssl)
$(eval $(call TEST_ONEFILE, hero_config_mcp, hero_config_mcp.cpp, $(HERO_CONFIG_LINK)))
$(TEST_OUT)/hero_config_mcp: INCLUDES_EXTRA += $(SSL_INCLUDE_PATHS)

.PHONY: all
all: $(TEST_OUT)/hero_config_mcp $(TEST_OUT)/hero_hashimyei_mcp $(TEST_OUT)/hero_lattice_mcp
	@$(LOG_SUCCESS)

.PHONY: run
run: $(TEST_OUT)/hero_config_mcp
	$(Q)$(RUN_MODE) $(TEST_OUT)/hero_config_mcp

.PHONY: install-hero-mcp
install-hero-mcp: $(TEST_OUT)/hero_config_mcp
	@chmod 700 $(TEST_OUT)/hero_config_mcp
	@printf "\033[2;33m[\033[90mUTIL: \033[2;32mBUILT\033[2;33m]\033[0m \033[90m%s\033[0m\n" "$(TEST_OUT)/hero_config_mcp"

.PHONY: install-hashimyei-tools
install-hashimyei-tools: $(TEST_OUT)/hero_hashimyei_mcp
	@chmod 700 $(TEST_OUT)/hero_hashimyei_mcp
	@printf "\033[2;33m[\033[90mUTIL: \033[2;32mBUILT\033[2;33m]\033[0m \033[90m%s\033[0m\n" "$(TEST_OUT)/hero_hashimyei_mcp"

.PHONY: install-lattice-tools
install-lattice-tools: $(TEST_OUT)/hero_lattice_mcp
	@chmod 700 $(TEST_OUT)/hero_lattice_mcp
	@printf "\033[2;33m[\033[90mUTIL: \033[2;32mBUILT\033[2;33m]\033[0m \033[90m%s\033[0m\n" "$(TEST_OUT)/hero_lattice_mcp"

.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/hero_config_mcp \
	  $(TEST_OUT)/hero_hashimyei_mcp \
	  $(TEST_OUT)/hero_lattice_mcp
