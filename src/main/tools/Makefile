# =============================================================================
# main/tools/Makefile — operational runtime utilities
# =============================================================================
ROOT_PATH := ../..
include $(ROOT_PATH)/Makefile.config

HERE_PATH := $(ROOT_PATH)/main/tools
# Standalone runtime utilities should live under .build/tools.
BIN_OUT  := $(TOOLS_OUT)

TEST_DEFAULT_LDLIBS :=
CUWACUNU_CAMPAIGN_LINK := $(libtorch_a) $(libcurl_a) $(libssl_a) $(libcommon_a) $(LDLIBS_torch) $(LDLIBS_curl) $(LDLIBS_ssl)
RUNTIME_RESET_LINK := $(libtorch_a) $(libcommon_a) $(LDLIBS_torch)
TSODAO_RELEASE_CXXFLAGS := $(filter-out $(DEBUG_FLAGS),$(CXXFLAGS)) -O2 -fno-omit-frame-pointer -D_GLIBCXX_ASSERTIONS -fstack-clash-protection -fno-plt
TSODAO_RELEASE_LDFLAGS := $(filter-out -rdynamic,$(LDFLAGS_E)) -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack -Wl,--as-needed
$(eval $(call BIN_ONEFILE, secure_key_setup, secure_key_setup.cpp, $(libssl_a) $(libcommon_a) $(libtorch_a) $(LDLIBS_ssl) $(LDLIBS_torch)))
$(eval $(call BIN_ONEFILE, cuwacunu_campaign, cuwacunu_campaign.cpp, $(CUWACUNU_CAMPAIGN_LINK)))
$(eval $(call BIN_ONEFILE, runtime_reset, runtime_reset.cpp, $(RUNTIME_RESET_LINK)))
$(eval $(call BIN_ONEFILE, tsodao, tsodao.cpp, ))

$(BIN_OUT)/secure_key_setup: INCLUDES_EXTRA += $(SSL_INCLUDE_PATHS)
$(BIN_OUT)/cuwacunu_campaign: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS)
$(BIN_OUT)/tsodao: CXXFLAGS := $(TSODAO_RELEASE_CXXFLAGS)
$(BIN_OUT)/tsodao: LDFLAGS_E := $(TSODAO_RELEASE_LDFLAGS)
$(BIN_OUT)/tsodao: POST_LINK_HOOK := strip --strip-unneeded $(BIN_OUT)/tsodao

define INSTALL_TOOL_BIN
.PHONY: install-$(strip $(1))
install-$(strip $(1)): $(BIN_OUT)/$(strip $(2))
	@mode="$$(stat -c '%a' "$(BIN_OUT)/$(strip $(2))" 2>/dev/null || echo missing)"; \
	if [ "$$mode" != "700" ]; then \
		chmod 700 "$(BIN_OUT)/$(strip $(2))"; \
		printf "\033[2;33m[\033[90mTOOL: \033[2;32mINSTALLED\033[2;33m]\033[0m \033[90m%s\033[0m\n" "$(BIN_OUT)/$(strip $(2))"; \
	fi
endef

$(eval $(call INSTALL_TOOL_BIN,secure-key-setup,secure_key_setup))
$(eval $(call INSTALL_TOOL_BIN,campaign,cuwacunu_campaign))
$(eval $(call INSTALL_TOOL_BIN,reset-runtime,runtime_reset))
$(eval $(call INSTALL_TOOL_BIN,tsodao,tsodao))

.PHONY: all
all: $(BIN_OUT)/secure_key_setup $(BIN_OUT)/cuwacunu_campaign $(BIN_OUT)/runtime_reset $(BIN_OUT)/tsodao
	@$(LOG_SUCCESS)

.PHONY: run
run: run-secure_key_setup

.PHONY: build-campaign
build-campaign: autoprep $(BIN_OUT)/cuwacunu_campaign
	@$(LOG_SUCCESS)

.PHONY: run-campaign
run-campaign: $(BIN_OUT)/cuwacunu_campaign
	$(Q)$(RUN_MODE) $(BIN_OUT)/cuwacunu_campaign

.PHONY: campaign
campaign: run-campaign

.PHONY: build-reset-runtime
build-reset-runtime: autoprep $(BIN_OUT)/runtime_reset
	@$(LOG_SUCCESS)

.PHONY: run-reset-runtime
run-reset-runtime: $(BIN_OUT)/runtime_reset
	$(Q)$(RUN_MODE) $(BIN_OUT)/runtime_reset

.PHONY: reset-runtime
reset-runtime: run-reset-runtime

.PHONY: build-tsodao
build-tsodao: $(BIN_OUT)/tsodao
	@$(LOG_SUCCESS)

.PHONY: clean
clean:
	$(Q)$(CLEAN_RM_CMD) $(BIN_OUT)/secure_key_setup
	$(Q)$(CLEAN_RM_CMD) $(BIN_OUT)/cuwacunu_campaign
	$(Q)$(CLEAN_RM_CMD) $(BIN_OUT)/runtime_reset
	$(Q)$(CLEAN_RM_CMD) $(BIN_OUT)/tsodao
