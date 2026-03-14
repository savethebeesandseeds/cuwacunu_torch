# =============================================================================
# main/Makefile — runtime executable aggregator
# =============================================================================
ROOT_PATH := ..
include $(ROOT_PATH)/Makefile.config

HERE_PATH := $(ROOT_PATH)/main
TESTS_PATH := $(ROOT_PATH)/tests
# Runtime utility binaries should live under .build/hero.
TEST_OUT := $(HERO_OUT)

TEST_DEFAULT_LDLIBS :=
MAIN_BOARD_LINK := $(libtorch_a) $(libcurl_a) $(libssl_a) $(libcommon_a) $(LDLIBS_torch) $(LDLIBS_curl) $(LDLIBS_ssl)
RUNTIME_RESET_LINK := $(libcommon_a)
$(eval $(call TEST_ONEFILE, main_board, main_board.cpp, $(MAIN_BOARD_LINK)))
$(eval $(call TEST_ONEFILE, runtime_reset, runtime_reset.cpp, $(RUNTIME_RESET_LINK)))
$(TEST_OUT)/main_board: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS)

PRODUCTION_RULES := all hero board hashimyei_hero runtime_hero jkimyei_hero tools config_hero install-hashimyei-tools install-runtime-tools install-jkimyei-tools clean
MAIN_LOCAL_TARGETS := all clean hero board hashimyei_hero runtime_hero jkimyei_hero tools config_hero install-hashimyei-tools install-runtime-tools install-jkimyei-tools tests test ta clean_tests \
	build-reset-runtime run-reset-runtime reset-runtime \
	parser dsl types exchange piaabo data iinuji iinuji_torch tsiemene iitepi \
	https db hashimyei jkimyei vicreg mdn expval
SRC_BRIDGE_TARGETS := all modules lib-bundles build b mods m bundles pack test \
	t tests main piaabo iitepi camahjucunu jkimyei wikimyei iinuji it \
	pia cam jk wk ii clean clean_all c ccommon cdsl cdata cwiki clean_common \
	clean_dsl clean_data clean_wikimyei
TARGET_RULES := $(sort \
	$(MAIN_LOCAL_TARGETS) \
	$(addprefix main_,$(MAIN_LOCAL_TARGETS)) \
	$(addprefix src_,$(SRC_BRIDGE_TARGETS)) \
	rules production_rules targets help)

.PHONY: hero
hero:
	+$(MAKE) -C $(HERE_PATH)/hero all

.PHONY: build-board
build-board: $(TEST_OUT)/main_board
	@$(LOG_SUCCESS)

.PHONY: run-board
run-board: $(TEST_OUT)/main_board
	$(Q)$(RUN_MODE) $(TEST_OUT)/main_board

.PHONY: board
board: run-board

.PHONY: build-reset-runtime
build-reset-runtime: $(TEST_OUT)/runtime_reset
	@$(LOG_SUCCESS)

.PHONY: run-reset-runtime
run-reset-runtime: $(TEST_OUT)/runtime_reset
	$(Q)$(RUN_MODE) $(TEST_OUT)/runtime_reset

.PHONY: reset-runtime
reset-runtime: run-reset-runtime

.PHONY: hashimyei_hero
hashimyei_hero:
	+$(MAKE) -C $(HERE_PATH)/hero all

.PHONY: runtime_hero
runtime_hero:
	+$(MAKE) -C $(HERE_PATH)/hero install-runtime-tools

.PHONY: jkimyei_hero
jkimyei_hero:
	+$(MAKE) -C $(HERE_PATH)/hero install-jkimyei-tools

.PHONY: tools
tools:
	+$(MAKE) -C $(HERE_PATH)/tools all

.PHONY: tests
tests:
	+$(MAKE) -C $(TESTS_PATH) all

define FORWARD_MAIN_ALIAS
.PHONY: main_$(1)
main_$(1):
	+$(MAKE) -C $(HERE_PATH) $(1)
endef
$(foreach t,$(MAIN_LOCAL_TARGETS),$(eval $(call FORWARD_MAIN_ALIAS,$(t))))

define FORWARD_SRC_ALIAS
.PHONY: src_$(1)
src_$(1):
	+$(MAKE) -C $(ROOT_PATH) $(1)
endef
$(foreach t,$(SRC_BRIDGE_TARGETS),$(eval $(call FORWARD_SRC_ALIAS,$(t))))

.PHONY: config_hero
config_hero:
	+$(MAKE) -C $(HERE_PATH)/hero install-hero-mcp

.PHONY: install-hashimyei-tools
install-hashimyei-tools:
	+$(MAKE) -C $(HERE_PATH)/hero install-hashimyei-tools

.PHONY: install-runtime-tools
install-runtime-tools:
	+$(MAKE) -C $(HERE_PATH)/hero install-runtime-tools

.PHONY: install-jkimyei-tools
install-jkimyei-tools:
	+$(MAKE) -C $(HERE_PATH)/hero install-jkimyei-tools

# Pass-through test dispatcher targets so root-level mkc usage can call tests
# directly (for example: mkc parser, mkc test_dconfig).
.PHONY: test ta clean_tests parser dsl types exchange piaabo data iinuji \
	iinuji_torch tsiemene iitepi https db hashimyei jkimyei vicreg mdn expval
test ta clean_tests parser dsl types exchange piaabo data iinuji \
iinuji_torch tsiemene iitepi https db hashimyei jkimyei vicreg mdn expval:
	+$(MAKE) -C $(TESTS_PATH) $@

.PHONY: test_% clear_% clear-test_%
test_% clear_% clear-test_%:
	+$(MAKE) -C $(TESTS_PATH) $@

.PHONY: all
all: hero tools
	@$(LOG_SUCCESS)

.PHONY: production_rules rules
production_rules rules:
	@printf "%s\n" $(PRODUCTION_RULES)

.PHONY: targets
targets:
	@printf "%s\n" $(TARGET_RULES)

.PHONY: help
help:
	@echo "main targets:"
	@echo "  all hero board reset-runtime hashimyei_hero runtime_hero jkimyei_hero tools config_hero install-hashimyei-tools install-runtime-tools install-jkimyei-tools tests clean"
	@echo "  test groups: parser dsl types exchange piaabo data iinuji ... vicreg"
	@echo "  aliases: main_<target> (local), src_<target> (bridge to src/Makefile)"
	@echo "  list targets: make -C $(HERE_PATH) targets"

.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/main_board
	@rm -f $(TEST_OUT)/runtime_reset
	+$(MAKE) -C $(HERE_PATH)/hero clean
	+$(MAKE) -C $(HERE_PATH)/tools clean
	+$(MAKE) -C $(TESTS_PATH) clean
