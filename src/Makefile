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
	  2>/dev/null || true

.PHONY: clean_bnf
clean_bnf:
	@shred -u \
	  $(OUTPUT_PATH)/common/BNF_*.d \
	  $(OUTPUT_PATH)/common/BNF_*.o \
	  $(OUTPUT_PATH)/common/BNF_*.a \
	  $(OUTPUT_PATH)/common/observation_pipeline.d \
		$(OUTPUT_PATH)/common/iinuji_renderings.d \
		$(OUTPUT_PATH)/common/training_components.d \
	  $(OUTPUT_PATH)/common/observation_pipeline.o \
		$(OUTPUT_PATH)/common/iinuji_renderings.o \
		$(OUTPUT_PATH)/common/training_components.o \
	  $(OUTPUT_PATH)/common/observation_pipeline.a \
		$(OUTPUT_PATH)/common/iinuji_renderings.a \
		$(OUTPUT_PATH)/common/training_components.a \
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
	  2>/dev/null || true
	@shred -u \
	  $(OUTPUT_PATH)/*.d          $(OUTPUT_PATH)/*.o          $(OUTPUT_PATH)/*.a \
	  2>/dev/null || true
	@shred -u \
	  $(OUTPUT_PATH)/.deps/*.d    $(OUTPUT_PATH)/.objs/*.o \
	  2>/dev/null || true
	@rm -rf \
		$(OUTPUT_PATH)/.deps \
		2>/dev/null || true
