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

HASHIMYEI_TARGETS := hashimyei_catalog \
	hero_hashimyei_tools

# -----------------------------------------------------------------
# Aggregate Target
# -----------------------------------------------------------------
.PHONY: all
all: $(HASHIMYEI_TARGETS)
	@$(LOG_SUCCESS)
