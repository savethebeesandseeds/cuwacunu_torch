# =============================================================================
# main/Makefile — runtime executable aggregator
# =============================================================================
ROOT_PATH := ..
include $(ROOT_PATH)/Makefile.config

HERE_PATH := $(ROOT_PATH)/main
TESTS_PATH := $(ROOT_PATH)/tests
SUBMAKE_NO_AUTOPREP := AUTO_PREP=0

PRODUCTION_RULES := all link install hero campaign config_hero hashimyei_hero lattice_hero runtime_hero super_hero human_hero tools clean
MAIN_LOCAL_TARGETS := all link install clean hero campaign config_hero hashimyei_hero lattice_hero runtime_hero super_hero human_hero tools tests test ta clean_tests \
	build-reset-runtime run-reset-runtime reset-runtime \
	parser dsl types exchange piaabo data iinuji iinuji_torch tsiemene iitepi \
	https db hashimyei jkimyei vicreg mdn expval
TARGET_RULES := $(sort $(MAIN_LOCAL_TARGETS) rules production_rules targets help)

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
	+$(MAKE) -C $(HERE_PATH)/hero install-hashimyei-hero $(SUBMAKE_NO_AUTOPREP)

.PHONY: config_hero
config_hero: autoprep
	+$(MAKE) -C $(HERE_PATH)/hero install-config-hero $(SUBMAKE_NO_AUTOPREP)

.PHONY: lattice_hero
lattice_hero: autoprep
	+$(MAKE) -C $(HERE_PATH)/hero install-lattice-hero $(SUBMAKE_NO_AUTOPREP)

.PHONY: runtime_hero
runtime_hero: autoprep build-campaign build-reset-runtime
	+$(MAKE) -C $(HERE_PATH)/hero install-runtime-hero $(SUBMAKE_NO_AUTOPREP)

.PHONY: super_hero
super_hero: autoprep
	+$(MAKE) -C $(HERE_PATH)/hero install-super-hero $(SUBMAKE_NO_AUTOPREP)

.PHONY: human_hero
human_hero: autoprep
	+$(MAKE) -C $(HERE_PATH)/hero install-human-hero $(SUBMAKE_NO_AUTOPREP)

.PHONY: tools
tools: autoprep
	+$(MAKE) -C $(HERE_PATH)/tools all $(SUBMAKE_NO_AUTOPREP)

.PHONY: tests
tests: autoprep
	+$(MAKE) -C $(TESTS_PATH) all $(SUBMAKE_NO_AUTOPREP)

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

.PHONY: link
link: autoprep hero tools
	@$(LOG_SUCCESS)

.PHONY: install
install: link
	+$(MAKE) -C $(HERE_PATH)/hero install-config-hero install-hashimyei-hero install-lattice-hero install-runtime-hero install-super-hero install-human-hero $(SUBMAKE_NO_AUTOPREP)
	+$(MAKE) -C $(HERE_PATH)/tools install-secure-key-setup install-campaign install-reset-runtime install-tsodao $(SUBMAKE_NO_AUTOPREP)
	@$(LOG_SUCCESS)

.PHONY: all
all: install

.PHONY: production_rules rules
production_rules rules:
	@printf "%s\n" $(PRODUCTION_RULES)

.PHONY: targets
targets:
	@printf "%s\n" $(TARGET_RULES)

.PHONY: help
help:
	@echo "main targets (advanced/internal):"
	@echo "  prefer 'make -C $(ROOT_PATH) lib', 'make -C $(ROOT_PATH) link', or 'make -C $(ROOT_PATH) install' for normal builds"
	@echo "  all link install hero build-campaign campaign build-reset-runtime reset-runtime config_hero hashimyei_hero lattice_hero runtime_hero super_hero human_hero"
	@echo "  tools tests clean"
	@echo "  'link' builds final linked binaries; 'install' finalizes HERO/tool surfaces in place"
	@echo "  leaf installs live in: $(HERE_PATH)/hero (install-*-hero), $(HERE_PATH)/tools (install-secure-key-setup, install-campaign, install-reset-runtime, install-tsodao)"
	@echo "  test groups: parser dsl types exchange piaabo data iinuji ... vicreg"
	@echo "  list targets: make -C $(HERE_PATH) targets"

.PHONY: clean
clean:
	+$(MAKE) -C $(HERE_PATH)/hero clean
	+$(MAKE) -C $(HERE_PATH)/tools clean
	+$(MAKE) -C $(TESTS_PATH) clean
