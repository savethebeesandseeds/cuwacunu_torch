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

LATTICE_TARGETS := lattice_catalog \
	hero_lattice_tools

# -----------------------------------------------------------------
# Aggregate Target
# -----------------------------------------------------------------
.PHONY: all
all: $(LATTICE_TARGETS)
	@$(LOG_SUCCESS)
