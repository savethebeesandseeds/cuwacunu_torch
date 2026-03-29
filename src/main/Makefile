# =============================================================================
# main/Makefile — runtime executable aggregator
# =============================================================================
ROOT_PATH := ..
include $(ROOT_PATH)/Makefile.config

HERE_PATH := $(ROOT_PATH)/main
TESTS_PATH := $(ROOT_PATH)/tests
SUBMAKE_NO_AUTOPREP := AUTO_PREP=0

PRODUCTION_RULES := all hero campaign hashimyei_hero runtime_hero tools config_hero install-hashimyei-tools install-runtime-tools clean
MAIN_LOCAL_TARGETS := all clean hero campaign hashimyei_hero runtime_hero tools config_hero install-hashimyei-tools install-runtime-tools tests test ta clean_tests \
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
hero: autoprep
	+$(MAKE) -C $(HERE_PATH)/hero all $(SUBMAKE_NO_AUTOPREP)

.PHONY: build-campaign
build-campaign: autoprep
	+$(MAKE) -C $(HERE_PATH)/tools build-campaign $(SUBMAKE_NO_AUTOPREP)

.PHONY: run-campaign
run-campaign: autoprep
	+$(MAKE) -C $(HERE_PATH)/tools run-campaign $(SUBMAKE_NO_AUTOPREP)

.PHONY: campaign
campaign: run-campaign

.PHONY: build-reset-runtime
build-reset-runtime: autoprep
	+$(MAKE) -C $(HERE_PATH)/tools build-reset-runtime $(SUBMAKE_NO_AUTOPREP)

.PHONY: run-reset-runtime
run-reset-runtime: autoprep
	+$(MAKE) -C $(HERE_PATH)/tools run-reset-runtime $(SUBMAKE_NO_AUTOPREP)

.PHONY: reset-runtime
reset-runtime: run-reset-runtime

.PHONY: hashimyei_hero
hashimyei_hero: autoprep
	+$(MAKE) -C $(HERE_PATH)/hero all $(SUBMAKE_NO_AUTOPREP)

.PHONY: runtime_hero
runtime_hero: autoprep build-campaign build-reset-runtime
	+$(MAKE) -C $(HERE_PATH)/hero install-runtime-tools $(SUBMAKE_NO_AUTOPREP)

.PHONY: tools
tools: autoprep
	+$(MAKE) -C $(HERE_PATH)/tools all $(SUBMAKE_NO_AUTOPREP)

.PHONY: tests
tests: autoprep
	+$(MAKE) -C $(TESTS_PATH) all $(SUBMAKE_NO_AUTOPREP)

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
config_hero: autoprep
	+$(MAKE) -C $(HERE_PATH)/hero install-hero-mcp $(SUBMAKE_NO_AUTOPREP)

.PHONY: install-hashimyei-tools
install-hashimyei-tools: autoprep
	+$(MAKE) -C $(HERE_PATH)/hero install-hashimyei-tools $(SUBMAKE_NO_AUTOPREP)

.PHONY: install-runtime-tools
install-runtime-tools: autoprep
	+$(MAKE) -C $(HERE_PATH)/hero install-runtime-tools $(SUBMAKE_NO_AUTOPREP)

# Pass-through test dispatcher targets so root-level mkc usage can call tests
# directly (for example: mkc parser, mkc test_dconfig).
.PHONY: test ta clean_tests parser dsl types exchange piaabo data iinuji \
	iinuji_torch tsiemene iitepi https db hashimyei jkimyei vicreg mdn expval
test ta parser dsl types exchange piaabo data iinuji \
iinuji_torch tsiemene iitepi https db hashimyei jkimyei vicreg mdn expval: autoprep
	+$(MAKE) -C $(TESTS_PATH) $@ $(SUBMAKE_NO_AUTOPREP)

clean_tests:
	+$(MAKE) -C $(TESTS_PATH) $@

.PHONY: test_% clear_% clear-test_%
test_%: autoprep
	+$(MAKE) -C $(TESTS_PATH) $@ $(SUBMAKE_NO_AUTOPREP)

clear_% clear-test_%:
	+$(MAKE) -C $(TESTS_PATH) $@

.PHONY: all
all: autoprep hero tools
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
	@echo "  all hero build-campaign campaign build-reset-runtime reset-runtime hashimyei_hero runtime_hero tools config_hero install-hashimyei-tools install-runtime-tools tests clean"
	@echo "  test groups: parser dsl types exchange piaabo data iinuji ... vicreg"
	@echo "  aliases: main_<target> (local), src_<target> (bridge to src/Makefile)"
	@echo "  list targets: make -C $(HERE_PATH) targets"

.PHONY: clean
clean:
	+$(MAKE) -C $(HERE_PATH)/hero clean
	+$(MAKE) -C $(HERE_PATH)/tools clean
	+$(MAKE) -C $(TESTS_PATH) clean
