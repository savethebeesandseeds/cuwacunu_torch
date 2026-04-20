# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/hero/lattice_hero
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))

# -----------------------------------------------------------------
# Build Rules
# -----------------------------------------------------------------
$(eval $(call BUILD_OBJ_INC, common, lattice_catalog, lattice_catalog))
$(eval $(call BUILD_OBJ_INC, common, hero_lattice_tools, hero_lattice_tools, $(TORCH_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS)))

LATTICE_CATALOG_PARTS := \
  $(HERE_PATH)/lattice_catalog_private_core.cpp \
  $(HERE_PATH)/lattice_catalog_private_store.cpp \
  $(HERE_PATH)/lattice_catalog_private_runtime.cpp \
  $(HERE_PATH)/lattice_catalog_private_views.cpp

HERO_LATTICE_TOOLS_PARTS := \
  $(HERE_PATH)/hero_lattice_tools_private_core.cpp \
  $(HERE_PATH)/hero_lattice_tools_private_handlers.cpp \
  $(HERE_PATH)/hero_lattice_tools_private_public.cpp

$(OUTPUT_PATH)/common/lattice_catalog.o: $(LATTICE_CATALOG_PARTS)
$(OUTPUT_PATH)/common/hero_lattice_tools.o: $(HERO_LATTICE_TOOLS_PARTS)

LATTICE_TARGETS := lattice_catalog \
	hero_lattice_tools

# -----------------------------------------------------------------
# Aggregate Target
# -----------------------------------------------------------------
.PHONY: all
all: $(LATTICE_TARGETS)
	@$(LOG_SUCCESS)
