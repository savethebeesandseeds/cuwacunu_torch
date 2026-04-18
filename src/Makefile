# -----------------------------------------------------------------
# impl/Makefile — recurses into submodules
# -----------------------------------------------------------------
ROOT_PATH := .
include $(ROOT_PATH)/Makefile.config
.DEFAULT_GOAL := lib
HERE_PATH := $(ROOT_PATH)
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))
# -----------------------------------------------------------------
# Module entry points (delegate to subtrees)
# -----------------------------------------------------------------
.PHONY: piaabo
piaabo:
	$(MAKE) -C $(IMPL_PATH)/piaabo all

.PHONY: iitepi
iitepi:
	$(MAKE) -C $(IMPL_PATH)/iitepi all

.PHONY: camahjucunu
camahjucunu:
	$(MAKE) -C $(IMPL_PATH)/camahjucunu all

.PHONY: jkimyei
jkimyei:
	$(MAKE) -C $(IMPL_PATH)/jkimyei all

.PHONY: wikimyei
wikimyei:
	$(MAKE) -C $(IMPL_PATH)/wikimyei all

.PHONY: hero_impl
hero_impl:
	$(MAKE) -C $(IMPL_PATH) hero

.PHONY: link
link:
	$(MAKE) -C $(ROOT_PATH)/main link

.PHONY: install
install:
	$(MAKE) -C $(ROOT_PATH)/main install


# -----------------------------------------------------------------
# Aggregate builds
#   - Run with: make -j12 lib|link|install   (control parallelism at CLI)
# -----------------------------------------------------------------

# Group the library modules behind a single prerequisite
.PHONY: modules
modules: iitepi piaabo camahjucunu jkimyei wikimyei hero_impl

# Ensure each bundle pack step runs *after* modules completes, while still
# allowing the bundle pack steps to run in parallel with each other.
lib-bundle-common lib-bundle-curl lib-bundle-ssl lib-bundle-torch: | modules

# Ensure packing runs after modules.
lib-bundles: modules

.PHONY: lib
lib: lib-bundles
	@$(LOG_SUCCESS)

.PHONY: help
help:
	@echo "src targets:"
	@echo "  lib   canonical library/core aggregate"
	@echo "  link  canonical final-linked executable aggregate"
	@echo "  install  finalize in-place hero/tool/interface outputs after link"
	@echo "        bundled archives now land under $(LIB_OUT)"
	@echo "related:"
	@echo "  tests modules lib-bundles clean"
	@echo "module subtrees:"
	@echo "  piaabo iitepi camahjucunu jkimyei wikimyei hero_impl"
	@echo "selective cleans:"
	@echo "  clean_common clean_dsl clean_data clean_wikimyei"
	@echo "advanced/internal:"
	@echo "  direct main tree: make -C $(ROOT_PATH)/main all"

.PHONY: targets
targets:
	@printf "%s\n" lib link install modules lib-bundles tests piaabo iitepi camahjucunu jkimyei wikimyei hero_impl clean clean_common clean_dsl clean_data clean_wikimyei help targets

# -----------------------------------------------------------------
# Tests
# -----------------------------------------------------------------
.PHONY: tests
tests:
	$(MAKE) -C $(TESTS_PATH) all

# -----------------------------------------------------------------
# Selective cleans
# -----------------------------------------------------------------
.PHONY: clean_common
clean_common:
	$(Q)$(CLEAN_SHRED_CMD) \
	  $(OUTPUT_PATH)/common/*.d \
	  $(OUTPUT_PATH)/common/*.o \
	  $(OUTPUT_PATH)/common/*.a \
	  $(OUTPUT_PATH)/common/iitepi/*.d \
	  $(OUTPUT_PATH)/common/iitepi/*.o \
	  $(OUTPUT_PATH)/common/iitepi/*.a \
	  $(OUTPUT_PATH)/.deps/common/iitepi.d \
	  $(OUTPUT_PATH)/.deps/common/iitepi/*.d \
	  $(OUTPUT_PATH)/common/dconfig/*.d \
	  $(OUTPUT_PATH)/common/dconfig/*.o \
	  $(OUTPUT_PATH)/common/dconfig/*.a \
	  $(OUTPUT_PATH)/.deps/common/dconfig.d \
	  $(OUTPUT_PATH)/.deps/common/dconfig/*.d

.PHONY: clean_dsl
clean_dsl:
	$(Q)$(CLEAN_SHRED_CMD) \
	  $(OUTPUT_PATH)/common/parser_types.d \
	  $(OUTPUT_PATH)/common/parser_types.o \
	  $(OUTPUT_PATH)/common/parser_types.a \
	  $(OUTPUT_PATH)/common/ast.d \
	  $(OUTPUT_PATH)/common/ast.o \
	  $(OUTPUT_PATH)/common/ast.a \
	  $(OUTPUT_PATH)/common/grammar_lexer.d \
	  $(OUTPUT_PATH)/common/grammar_lexer.o \
	  $(OUTPUT_PATH)/common/grammar_lexer.a \
	  $(OUTPUT_PATH)/common/grammar_parser.d \
	  $(OUTPUT_PATH)/common/grammar_parser.o \
	  $(OUTPUT_PATH)/common/grammar_parser.a \
	  $(OUTPUT_PATH)/common/instruction_lexer.d \
	  $(OUTPUT_PATH)/common/instruction_lexer.o \
	  $(OUTPUT_PATH)/common/instruction_lexer.a \
	  $(OUTPUT_PATH)/common/instruction_parser.d \
	  $(OUTPUT_PATH)/common/instruction_parser.o \
	  $(OUTPUT_PATH)/common/instruction_parser.a \
	  $(OUTPUT_PATH)/common/canonical_path.d \
	  $(OUTPUT_PATH)/common/canonical_path.o \
	  $(OUTPUT_PATH)/common/canonical_path.a \
	  $(OUTPUT_PATH)/common/jkimyei_specs.d \
	  $(OUTPUT_PATH)/common/jkimyei_specs.o \
	  $(OUTPUT_PATH)/common/jkimyei_specs.a \
	  $(OUTPUT_PATH)/common/observation_spec.d \
	  $(OUTPUT_PATH)/common/observation_spec.o \
	  $(OUTPUT_PATH)/common/observation_spec.a \
	  $(OUTPUT_PATH)/common/observation_sources_decoder.d \
	  $(OUTPUT_PATH)/common/observation_sources_decoder.o \
	  $(OUTPUT_PATH)/common/observation_sources_decoder.a \
	  $(OUTPUT_PATH)/common/observation_channels_decoder.d \
	  $(OUTPUT_PATH)/common/observation_channels_decoder.o \
	  $(OUTPUT_PATH)/common/observation_channels_decoder.a \
	  $(OUTPUT_PATH)/common/tsiemene_circuit_runtime_invoke.d \
	  $(OUTPUT_PATH)/common/tsiemene_circuit_runtime_invoke.o \
	  $(OUTPUT_PATH)/common/tsiemene_circuit_runtime_invoke.a \
	  $(OUTPUT_PATH)/common/tsiemene_circuit_runtime_resolve.d \
	  $(OUTPUT_PATH)/common/tsiemene_circuit_runtime_resolve.o \
	  $(OUTPUT_PATH)/common/tsiemene_circuit_runtime_resolve.a \
	  $(OUTPUT_PATH)/common/tsiemene_circuit_runtime_validate.d \
	  $(OUTPUT_PATH)/common/tsiemene_circuit_runtime_validate.o \
	  $(OUTPUT_PATH)/common/tsiemene_circuit_runtime_validate.a \
	  $(OUTPUT_PATH)/common/tsiemene_circuit_decode_text.d \
	  $(OUTPUT_PATH)/common/tsiemene_circuit_decode_text.o \
	  $(OUTPUT_PATH)/common/tsiemene_circuit_decode_text.a \
	  $(OUTPUT_PATH)/common/tsiemene_circuit_decode_ast.d \
	  $(OUTPUT_PATH)/common/tsiemene_circuit_decode_ast.o \
	  $(OUTPUT_PATH)/common/tsiemene_circuit_decode_ast.a \
	  $(OUTPUT_PATH)/common/iitepi_wave.d \
	  $(OUTPUT_PATH)/common/iitepi_wave.o \
	  $(OUTPUT_PATH)/common/iitepi_wave.a

.PHONY: clean_data
clean_data:
	$(Q)$(CLEAN_SHRED_CMD) \
	  $(OUTPUT_PATH)/libtorch/memory_mapped_*.d \
	  $(OUTPUT_PATH)/libtorch/memory_mapped_*.o \
	  $(OUTPUT_PATH)/libtorch/memory_mapped_*.a

.PHONY: clean_wikimyei
clean_wikimyei:
	$(Q)$(CLEAN_SHRED_CMD) \
	  $(OUTPUT_PATH)/libtorch/vicreg_4d.d \
	  $(OUTPUT_PATH)/libtorch/vicreg_4d.o \
	  $(OUTPUT_PATH)/libtorch/vicreg_4d.a

# -----------------------------------------------------------------
# Full clean (interactive) — wipes all object, dep, and archive files
# -----------------------------------------------------------------
.PHONY: clean
clean:
	@read -p "Are you sure you want to clean the entire build? (y/N): " ans; \
	if [ "$$ans" != "y" ] && [ "$$ans" != "Y" ]; then \
		echo "Aborting clean."; exit 1; fi
	@echo "$(COLOR_RED) Cleaning cuwacunu build $(COLOR_RESET)"
	$(Q)$(CLEAN_TREE_CMD) "$(OUTPUT_PATH)" "$(LIB_OUT)" "$(TEST_OUT)" "$(HERO_OUT)" "$(TOOLS_OUT)" "$(INTERFACE_OUT)"
