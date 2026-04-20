# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/hero/hashimyei_hero
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))

# -----------------------------------------------------------------
# Build Rules
# -----------------------------------------------------------------
$(eval $(call BUILD_OBJ_INC, common, hashimyei_catalog, hashimyei_catalog))
$(eval $(call BUILD_OBJ_INC, common, hero_hashimyei_tools, hero_hashimyei_tools))

HASHIMYEI_CATALOG_PARTS := \
  $(HERE_PATH)/hashimyei_catalog_private_core.cpp \
  $(HERE_PATH)/hashimyei_catalog_private_manifests.cpp \
  $(HERE_PATH)/hashimyei_catalog_private_ingest.cpp \
  $(HERE_PATH)/hashimyei_catalog_private_queries.cpp

HERO_HASHIMYEI_TOOLS_PARTS := \
  $(HERE_PATH)/hero_hashimyei_tools_private_core.cpp \
  $(HERE_PATH)/hero_hashimyei_tools_private_handlers.cpp \
  $(HERE_PATH)/hero_hashimyei_tools_private_public.cpp

$(OUTPUT_PATH)/common/hashimyei_catalog.o: $(HASHIMYEI_CATALOG_PARTS)
$(OUTPUT_PATH)/common/hero_hashimyei_tools.o: $(HERO_HASHIMYEI_TOOLS_PARTS)

HASHIMYEI_TARGETS := hashimyei_catalog \
	hero_hashimyei_tools

# -----------------------------------------------------------------
# Aggregate Target
# -----------------------------------------------------------------
.PHONY: all
all: $(HASHIMYEI_TARGETS)
	@$(LOG_SUCCESS)
