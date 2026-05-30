ROOT_PATH := ../..
include $(ROOT_PATH)/Makefile.config

HERE_PATH := $(ROOT_PATH)/main/interface
BIN_OUT := $(INTERFACE_OUT)
INSTALL_BIN_DIR ?= /usr/local/bin

TEST_DEFAULT_LDLIBS :=
CUWACUNU_CMD_LDLIBS := $(LDLIBS_ncurses) -L/usr/lib/x86_64-linux-gnu -l:libz.so.1
CUWACUNU_CMD_ALIASES := cuwacunu.cmd iinuji_cmd

$(eval $(call BIN_ONEFILE,cuwacunu_cmd,cuwacunu_cmd.cpp,$(CUWACUNU_CMD_LDLIBS)))

$(BIN_OUT)/cuwacunu_cmd: INCLUDES_EXTRA += $(NCURSES_INCLUDE_PATHS)

$(addprefix $(BIN_OUT)/,$(CUWACUNU_CMD_ALIASES)): $(BIN_OUT)/cuwacunu_cmd
	$(Q)ln -sf cuwacunu_cmd "$@"

.PHONY: all build-cuwacunu-cmd run-cuwacunu-cmd snapshot-cuwacunu-cmd install-cuwacunu-cmd cuwacunu.cmd iinuji_cmd clean

all build-cuwacunu-cmd: $(BIN_OUT)/cuwacunu_cmd $(addprefix $(BIN_OUT)/,$(CUWACUNU_CMD_ALIASES))
	@$(LOG_SUCCESS)

run-cuwacunu-cmd: $(BIN_OUT)/cuwacunu_cmd
	$(Q)$(RUN_MODE) $(BIN_OUT)/cuwacunu_cmd

cuwacunu.cmd: $(BIN_OUT)/cuwacunu.cmd
	$(Q)$(RUN_MODE) "$<"

iinuji_cmd: $(BIN_OUT)/iinuji_cmd
	$(Q)$(RUN_MODE) "$<"

snapshot-cuwacunu-cmd: $(BIN_OUT)/cuwacunu_cmd
	$(Q)$(RUN_MODE) $(BIN_OUT)/cuwacunu_cmd --snapshot

install-cuwacunu-cmd: $(BIN_OUT)/cuwacunu_cmd $(addprefix $(BIN_OUT)/,$(CUWACUNU_CMD_ALIASES))
	@mode="$$(stat -c '%a' "$(BIN_OUT)/cuwacunu_cmd" 2>/dev/null || echo missing)"; \
	if [ "$$mode" != "755" ]; then \
		chmod 755 "$(BIN_OUT)/cuwacunu_cmd"; \
		printf "\033[2;33m[\033[90mINTERFACE: \033[2;32mINSTALLED\033[2;33m]\033[0m \033[90m%s\033[0m\n" "$(BIN_OUT)/cuwacunu_cmd"; \
	fi; \
	target="$$(realpath "$(BIN_OUT)/cuwacunu_cmd")"; \
	mkdir -p "$(INSTALL_BIN_DIR)"; \
	ln -sfn "$$target" "$(INSTALL_BIN_DIR)/cuwacunu_cmd"; \
	for alias in $(CUWACUNU_CMD_ALIASES); do \
		ln -sfn "$$target" "$(INSTALL_BIN_DIR)/$$alias"; \
	done; \
	printf "\033[2;33m[\033[90mINTERFACE: \033[2;32mPATH\033[2;33m]\033[0m \033[90m%s -> %s\033[0m\n" "$(INSTALL_BIN_DIR)/cuwacunu_cmd" "$$target"

clean:
	$(Q)$(CLEAN_RM_CMD) $(BIN_OUT)/cuwacunu_cmd $(addprefix $(BIN_OUT)/,$(CUWACUNU_CMD_ALIASES)) $(OBJTMP)/cuwacunu_cmd.o
