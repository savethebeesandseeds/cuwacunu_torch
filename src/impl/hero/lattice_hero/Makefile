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

LATTICE_OBJS := $(OUTPUT_PATH)/common/lattice_catalog.o \
	$(OUTPUT_PATH)/common/hero_lattice_tools.o

# -----------------------------------------------------------------
# Aggregate Target
# -----------------------------------------------------------------
.PHONY: all
all: $(LATTICE_OBJS)
	@$(LOG_SUCCESS)
