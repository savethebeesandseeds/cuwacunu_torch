ROOT_PATH := ../..
include $(ROOT_PATH)/Makefile.config

HERE_PATH := $(ROOT_PATH)/main/interface
BIN_OUT := $(INTERFACE_OUT)
INSTALL_BIN_DIR ?= /usr/local/bin

TEST_DEFAULT_LDLIBS :=
CUWACUNU_CMD_LDLIBS := $(LDLIBS_ncurses) -L/usr/lib/x86_64-linux-gnu -l:libz.so.1
CUWACUNU_CMD_ALIASES := cuwacunu.cmd iinuji_cmd
IINUJI_HTML_ALIASES := cuwacunu_html
IINUJI_HTML_BIND ?= 127.0.0.1
IINUJI_HTML_PORT ?= 8765

$(eval $(call BIN_ONEFILE,cuwacunu_cmd,cuwacunu_cmd.cpp,$(CUWACUNU_CMD_LDLIBS)))
$(eval $(call BIN_ONEFILE,iinuji_html,iinuji_html.cpp,))

$(BIN_OUT)/cuwacunu_cmd: INCLUDES_EXTRA += $(NCURSES_INCLUDE_PATHS)

$(addprefix $(BIN_OUT)/,$(CUWACUNU_CMD_ALIASES)): $(BIN_OUT)/cuwacunu_cmd
	$(Q)ln -sf cuwacunu_cmd "$@"

$(addprefix $(BIN_OUT)/,$(IINUJI_HTML_ALIASES)): $(BIN_OUT)/iinuji_html
	$(Q)ln -sf iinuji_html "$@"

.PHONY: all build-cuwacunu-cmd build-iinuji-html run-cuwacunu-cmd run-iinuji-html validate-iinuji-html snapshot-cuwacunu-cmd install-cuwacunu-cmd install-iinuji-html cuwacunu.cmd iinuji_cmd clean

all: build-cuwacunu-cmd build-iinuji-html
	@$(LOG_SUCCESS)

build-cuwacunu-cmd: $(BIN_OUT)/cuwacunu_cmd $(addprefix $(BIN_OUT)/,$(CUWACUNU_CMD_ALIASES))
	@$(LOG_SUCCESS)

build-iinuji-html: $(BIN_OUT)/iinuji_html $(addprefix $(BIN_OUT)/,$(IINUJI_HTML_ALIASES))
	@$(LOG_SUCCESS)

run-cuwacunu-cmd: $(BIN_OUT)/cuwacunu_cmd
	$(Q)$(RUN_MODE) $(BIN_OUT)/cuwacunu_cmd

run-iinuji-html: $(BIN_OUT)/iinuji_html
	$(Q)cd "$(abspath $(ROOT_PATH)/..)" && \
		$(RUN_MODE) "$(abspath $(BIN_OUT)/iinuji_html)" \
		--repo-root "$(abspath $(ROOT_PATH)/..)" \
		--bind "$(IINUJI_HTML_BIND)" --port "$(IINUJI_HTML_PORT)"

validate-iinuji-html: $(BIN_OUT)/iinuji_html
	$(Q)cd "$(abspath $(ROOT_PATH)/..)" && \
		$(RUN_MODE) "$(abspath $(BIN_OUT)/iinuji_html)" \
		--repo-root "$(abspath $(ROOT_PATH)/..)" --validate-only

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

install-iinuji-html: $(BIN_OUT)/iinuji_html $(addprefix $(BIN_OUT)/,$(IINUJI_HTML_ALIASES))
	@mode="$$(stat -c '%a' "$(BIN_OUT)/iinuji_html" 2>/dev/null || echo missing)"; \
	if [ "$$mode" != "755" ]; then \
		chmod 755 "$(BIN_OUT)/iinuji_html"; \
		printf "\033[2;33m[\033[90mINTERFACE: \033[2;32mINSTALLED\033[2;33m]\033[0m \033[90m%s\033[0m\n" "$(BIN_OUT)/iinuji_html"; \
	fi; \
	target="$$(realpath "$(BIN_OUT)/iinuji_html")"; \
	mkdir -p "$(INSTALL_BIN_DIR)"; \
	ln -sfn "$$target" "$(INSTALL_BIN_DIR)/iinuji_html"; \
	for alias in $(IINUJI_HTML_ALIASES); do \
		ln -sfn "$$target" "$(INSTALL_BIN_DIR)/$$alias"; \
	done; \
	printf "\033[2;33m[\033[90mINTERFACE: \033[2;32mPATH\033[2;33m]\033[0m \033[90m%s -> %s\033[0m\n" "$(INSTALL_BIN_DIR)/iinuji_html" "$$target"

clean:
	$(Q)$(CLEAN_RM_CMD) $(BIN_OUT)/cuwacunu_cmd $(addprefix $(BIN_OUT)/,$(CUWACUNU_CMD_ALIASES)) $(OBJTMP)/cuwacunu_cmd.o
	$(Q)$(CLEAN_RM_CMD) $(BIN_OUT)/iinuji_html $(addprefix $(BIN_OUT)/,$(IINUJI_HTML_ALIASES)) $(OBJTMP)/iinuji_html.o
