# =============================================================================
# cuwacunu/tools/Makefile — operational runtime utilities
# =============================================================================
ROOT_PATH := ../..
include $(ROOT_PATH)/Makefile.config

HERE_PATH := $(ROOT_PATH)/cuwacunu/tools
# Runtime utility binaries should live under src/cuwacunu/build.
TEST_OUT := $(ROOT_PATH)/cuwacunu/build

TEST_DEFAULT_LDLIBS :=
$(eval $(call TEST_ONEFILE, secure_key_setup, secure_key_setup.cpp, $(libssl_a) $(libcommon_a) $(libtorch_a) $(LDLIBS_ssl) $(LDLIBS_torch)))

$(TEST_OUT)/secure_key_setup: INCLUDES_EXTRA += $(SSL_INCLUDE_PATHS)

CONFIG_SECURE_KEY_SETUP_BIN := $(ROOT_PATH)/config/secure_key_setup

.PHONY: all
all: $(TEST_OUT)/secure_key_setup
	@$(LOG_SUCCESS)

.PHONY: run
run: run-secure_key_setup

.PHONY: install-secure-key-setup
install-secure-key-setup: $(TEST_OUT)/secure_key_setup
	@cp -f $(TEST_OUT)/secure_key_setup $(CONFIG_SECURE_KEY_SETUP_BIN)
	@chmod 700 $(CONFIG_SECURE_KEY_SETUP_BIN)
	@printf "\033[2;33m[\033[90mUTIL: \033[2;32mINSTALLED\033[2;33m]\033[0m \033[90m%s\033[0m\n" "$(CONFIG_SECURE_KEY_SETUP_BIN)"

.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/secure_key_setup
