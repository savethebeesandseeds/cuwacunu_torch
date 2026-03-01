# -----------------------------------------------------------------
# impl/Makefile — recurses into submodules
# -----------------------------------------------------------------
ROOT_PATH := .
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(ROOT_PATH)
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))
# -----------------------------------------------------------------
# Module entry points (delegate to subtrees)
# -----------------------------------------------------------------
.PHONY: piaabo
piaabo:
	$(MAKE) -C $(IMPL_PATH)/piaabo all

.PHONY: camahjucunu
camahjucunu:
	$(MAKE) -C $(IMPL_PATH)/camahjucunu all

.PHONY: jkimyei
jkimyei:
	$(MAKE) -C $(IMPL_PATH)/jkimyei all

.PHONY: wikimyei
wikimyei:
	$(MAKE) -C $(IMPL_PATH)/wikimyei all

.PHONY: iinuji
iinuji:
	$(MAKE) -C $(IMPL_PATH)/iinuji all


# -----------------------------------------------------------------
# Aggregate builds
#   - Run with: make -j all   (control parallelism at CLI)
# -----------------------------------------------------------------

# Group the modules behind a single prerequisite
.PHONY: modules
modules: piaabo camahjucunu jkimyei wikimyei

# Ensure packing runs *after* modules
lib-bundles: modules

.PHONY: all
all: modules lib-bundles
	@$(LOG_SUCCESS)

# -----------------------------------------------------------------
# Shortcut aliases (delegate only; no build logic changes)
# -----------------------------------------------------------------
.PHONY: build b mods m bundles pack test t
build b: all
mods m: modules
bundles pack: lib-bundles
test t: tests

.PHONY: pia cam jk wk ii
pia: piaabo
cam: camahjucunu
jk: jkimyei
wk: wikimyei
ii: iinuji

.PHONY: c clean_all ccommon cdsl cdata cwiki
c clean_all: clean
ccommon: clean_common
cdsl: clean_dsl
cdata: clean_data
cwiki: clean_wikimyei

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
	@shred -u \
	  $(OUTPUT_PATH)/common/*.d \
	  $(OUTPUT_PATH)/common/*.o \
	  $(OUTPUT_PATH)/common/*.a \
	  $(OUTPUT_PATH)/common/dconfig/*.d \
	  $(OUTPUT_PATH)/common/dconfig/*.o \
	  $(OUTPUT_PATH)/common/dconfig/*.a \
	  $(OUTPUT_PATH)/.deps/common/dconfig.d \
	  $(OUTPUT_PATH)/.deps/common/dconfig/*.d \
	  2>/dev/null || true

.PHONY: clean_dsl
clean_dsl:
	@shred -u \
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
	  $(OUTPUT_PATH)/common/tsiemene_wave.d \
	  $(OUTPUT_PATH)/common/tsiemene_wave.o \
	  $(OUTPUT_PATH)/common/tsiemene_wave.a \
	  2>/dev/null || true

.PHONY: clean_data
clean_data:
	@shred -u \
	  $(OUTPUT_PATH)/libtorch/memory_mapped_*.d \
	  $(OUTPUT_PATH)/libtorch/memory_mapped_*.o \
	  $(OUTPUT_PATH)/libtorch/memory_mapped_*.a \
	  2>/dev/null || true

.PHONY: clean_wikimyei
clean_wikimyei:
	@shred -u \
	  $(OUTPUT_PATH)/libtorch/vicreg_4d.d \
	  $(OUTPUT_PATH)/libtorch/vicreg_4d.o \
	  $(OUTPUT_PATH)/libtorch/vicreg_4d.a \
	  2>/dev/null || true

# -----------------------------------------------------------------
# Full clean (interactive) — wipes all object, dep, and archive files
# -----------------------------------------------------------------
.PHONY: clean
clean:
	@read -p "Are you sure you want to clean the entire build? (y/N): " ans; \
	if [ "$$ans" != "y" ] && [ "$$ans" != "Y" ]; then \
		echo "Aborting clean."; exit 1; fi
	@echo "$(COLOR_RED) Cleaning cuwacunu build $(COLOR_RESET)"
	@shred -u \
	  $(OUTPUT_PATH)/libcurl/*.d  $(OUTPUT_PATH)/libcurl/*.o  $(OUTPUT_PATH)/libcurl/*.a \
	  2>/dev/null || true
	@shred -u \
	  $(OUTPUT_PATH)/libtorch/*.d $(OUTPUT_PATH)/libtorch/*.o $(OUTPUT_PATH)/libtorch/*.a \
	  2>/dev/null || true
	@shred -u \
	  $(OUTPUT_PATH)/openssl/*.d  $(OUTPUT_PATH)/openssl/*.o  $(OUTPUT_PATH)/openssl/*.a \
	  2>/dev/null || true
	@shred -u \
	  $(OUTPUT_PATH)/common/*.d   $(OUTPUT_PATH)/common/*.o   $(OUTPUT_PATH)/common/*.a \
	  $(OUTPUT_PATH)/common/dconfig/*.d $(OUTPUT_PATH)/common/dconfig/*.o $(OUTPUT_PATH)/common/dconfig/*.a \
	  2>/dev/null || true
	@shred -u \
	  $(OUTPUT_PATH)/*.d          $(OUTPUT_PATH)/*.o          $(OUTPUT_PATH)/*.a \
	  2>/dev/null || true
	@shred -u \
	  $(OUTPUT_PATH)/.deps/*.d    $(OUTPUT_PATH)/.deps/common/dconfig.d $(OUTPUT_PATH)/.deps/common/dconfig/*.d $(OUTPUT_PATH)/.objs/*.o \
	  2>/dev/null || true
	@rm -rf \
		$(OUTPUT_PATH)/.deps \
		2>/dev/null || true
