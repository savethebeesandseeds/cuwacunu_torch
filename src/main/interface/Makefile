# =============================================================================
# main/interface/Makefile — interactive/operator interfaces
# =============================================================================
ROOT_PATH := ../..
include $(ROOT_PATH)/Makefile.config

HERE_PATH := $(ROOT_PATH)/main/interface
BIN_OUT := $(INTERFACE_OUT)

TEST_DEFAULT_LDLIBS :=
IINUJI_CMD_LINK := $(libtorch_a) $(libcurl_a) $(libssl_a) $(libcommon_a) $(LDLIBS_torch) $(LDLIBS_curl) $(LDLIBS_ssl) $(LDLIBS_ncurses)

$(eval $(call BIN_ONEFILE, iinuji_cmd, iinuji_cmd.cpp, $(IINUJI_CMD_LINK)))
$(BIN_OUT)/iinuji_cmd: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS) $(NCURSES_INCLUDE_PATHS)
$(BIN_OUT)/iinuji_cmd: LDLIBS_EXTRA += -L/lib/x86_64-linux-gnu -L/usr/lib/x86_64-linux-gnu -l:libz.so.1

define INSTALL_INTERFACE_BIN
.PHONY: install-$(strip $(1))
install-$(strip $(1)): $(BIN_OUT)/$(strip $(2))
	@mode="$$(stat -c '%a' "$(BIN_OUT)/$(strip $(2))" 2>/dev/null || echo missing)"; \
	if [ "$$mode" != "700" ]; then \
		chmod 700 "$(BIN_OUT)/$(strip $(2))"; \
		printf "\033[2;33m[\033[90mCMD:  \033[2;32mINSTALLED\033[2;33m]\033[0m \033[90m%s\033[0m\n" "$(BIN_OUT)/$(strip $(2))"; \
	fi
endef

$(eval $(call INSTALL_INTERFACE_BIN,iinuji-cmd,iinuji_cmd))

.PHONY: all
all: $(BIN_OUT)/iinuji_cmd
	@$(LOG_SUCCESS)

.PHONY: run
run: run-iinuji_cmd

.PHONY: build-iinuji-cmd
build-iinuji-cmd: autoprep $(BIN_OUT)/iinuji_cmd
	@$(LOG_SUCCESS)

.PHONY: run-iinuji-cmd
run-iinuji-cmd: $(BIN_OUT)/iinuji_cmd
	$(Q)$(RUN_MODE) $(BIN_OUT)/iinuji_cmd

.PHONY: iinuji_cmd
iinuji_cmd: run-iinuji-cmd

.PHONY: clean
clean:
	$(Q)$(CLEAN_RM_CMD) $(BIN_OUT)/iinuji_cmd
