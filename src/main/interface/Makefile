# =============================================================================
# main/interface/Makefile — interactive/operator interfaces
# =============================================================================
ROOT_PATH := ../..
include $(ROOT_PATH)/Makefile.config

HERE_PATH := $(ROOT_PATH)/main/interface
BIN_OUT := $(INTERFACE_OUT)

TEST_DEFAULT_LDLIBS :=
IINUJI_CMD_LINK := $(libtorch_a) $(libcurl_a) $(libssl_a) $(libcommon_a) $(LDLIBS_torch) $(LDLIBS_curl) $(LDLIBS_ssl) $(LDLIBS_ncurses)
CUWACUNU_CMD_BIN := cuwacunu.cmd
CUWACUNU_CMD_SRC := cuwacunu_cmd.cpp

$(eval $(call BIN_ONEFILE, $(CUWACUNU_CMD_BIN), $(CUWACUNU_CMD_SRC), $(IINUJI_CMD_LINK)))
$(BIN_OUT)/$(CUWACUNU_CMD_BIN): INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS) $(NCURSES_INCLUDE_PATHS)
$(BIN_OUT)/$(CUWACUNU_CMD_BIN): LDLIBS_EXTRA += -L/lib/x86_64-linux-gnu -L/usr/lib/x86_64-linux-gnu -l:libz.so.1

define INSTALL_INTERFACE_BIN
.PHONY: install-$(strip $(1))
install-$(strip $(1)): $(BIN_OUT)/$(strip $(2))
	@mode="$$(stat -c '%a' "$(BIN_OUT)/$(strip $(2))" 2>/dev/null || echo missing)"; \
	if [ "$$mode" != "700" ]; then \
		chmod 700 "$(BIN_OUT)/$(strip $(2))"; \
		printf "\033[2;33m[\033[90mCMD:  \033[2;32mINSTALLED\033[2;33m]\033[0m \033[90m%s\033[0m\n" "$(BIN_OUT)/$(strip $(2))"; \
	fi
endef

$(eval $(call INSTALL_INTERFACE_BIN,cuwacunu-cmd,$(CUWACUNU_CMD_BIN)))

.PHONY: all
all: $(BIN_OUT)/$(CUWACUNU_CMD_BIN)
	@$(LOG_SUCCESS)

.PHONY: run
run: run-cuwacunu-cmd

.PHONY: build-cuwacunu-cmd
build-cuwacunu-cmd: $(ORDER_PREP) $(BIN_OUT)/$(CUWACUNU_CMD_BIN)
	@$(LOG_SUCCESS)

.PHONY: run-cuwacunu-cmd
run-cuwacunu-cmd: $(BIN_OUT)/$(CUWACUNU_CMD_BIN)
	$(Q)$(RUN_MODE) $(BIN_OUT)/$(CUWACUNU_CMD_BIN)

.PHONY: cuwacunu.cmd
cuwacunu.cmd: run-cuwacunu-cmd

.PHONY: clean
clean:
	$(Q)$(CLEAN_RM_CMD) $(BIN_OUT)/$(CUWACUNU_CMD_BIN)
