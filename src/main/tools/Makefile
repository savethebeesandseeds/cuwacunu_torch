# =============================================================================
# main/tools/Makefile — operational runtime utilities
# =============================================================================
ROOT_PATH := ../..
include $(ROOT_PATH)/Makefile.config

HERE_PATH := $(ROOT_PATH)/main/tools
# Standalone runtime utilities should live under .build/tools.
TEST_OUT := $(TOOLS_OUT)

TEST_DEFAULT_LDLIBS :=
CUWACUNU_CAMPAIGN_LINK := $(libtorch_a) $(libcurl_a) $(libssl_a) $(libcommon_a) $(LDLIBS_torch) $(LDLIBS_curl) $(LDLIBS_ssl)
RUNTIME_RESET_LINK := $(libtorch_a) $(libcommon_a) $(LDLIBS_torch)
$(eval $(call TEST_ONEFILE, secure_key_setup, secure_key_setup.cpp, $(libssl_a) $(libcommon_a) $(libtorch_a) $(LDLIBS_ssl) $(LDLIBS_torch)))
$(eval $(call TEST_ONEFILE, cuwacunu_campaign, cuwacunu_campaign.cpp, $(CUWACUNU_CAMPAIGN_LINK)))
$(eval $(call TEST_ONEFILE, runtime_reset, runtime_reset.cpp, $(RUNTIME_RESET_LINK)))

$(TEST_OUT)/secure_key_setup: INCLUDES_EXTRA += $(SSL_INCLUDE_PATHS)
$(TEST_OUT)/cuwacunu_campaign: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS)

CONFIG_SECURE_KEY_SETUP_BIN := $(ROOT_PATH)/config/secure_key_setup

.PHONY: all
all: $(TEST_OUT)/secure_key_setup $(TEST_OUT)/cuwacunu_campaign $(TEST_OUT)/runtime_reset
	@$(LOG_SUCCESS)

.PHONY: run
run: run-secure_key_setup

.PHONY: build-campaign
build-campaign: autoprep $(TEST_OUT)/cuwacunu_campaign
	@$(LOG_SUCCESS)

.PHONY: run-campaign
run-campaign: $(TEST_OUT)/cuwacunu_campaign
	$(Q)$(RUN_MODE) $(TEST_OUT)/cuwacunu_campaign

.PHONY: campaign
campaign: run-campaign

.PHONY: build-reset-runtime
build-reset-runtime: autoprep $(TEST_OUT)/runtime_reset
	@$(LOG_SUCCESS)

.PHONY: run-reset-runtime
run-reset-runtime: $(TEST_OUT)/runtime_reset
	$(Q)$(RUN_MODE) $(TEST_OUT)/runtime_reset

.PHONY: reset-runtime
reset-runtime: run-reset-runtime

.PHONY: install-secure-key-setup
install-secure-key-setup: $(TEST_OUT)/secure_key_setup
	@cp -f $(TEST_OUT)/secure_key_setup $(CONFIG_SECURE_KEY_SETUP_BIN)
	@chmod 700 $(CONFIG_SECURE_KEY_SETUP_BIN)
	@printf "\033[2;33m[\033[90mUTIL: \033[2;32mINSTALLED\033[2;33m]\033[0m \033[90m%s\033[0m\n" "$(CONFIG_SECURE_KEY_SETUP_BIN)"

.PHONY: clean
clean:
	$(Q)$(CLEAN_RM_CMD) $(TEST_OUT)/secure_key_setup
	$(Q)$(CLEAN_RM_CMD) $(TEST_OUT)/cuwacunu_campaign
	$(Q)$(CLEAN_RM_CMD) $(TEST_OUT)/runtime_reset
