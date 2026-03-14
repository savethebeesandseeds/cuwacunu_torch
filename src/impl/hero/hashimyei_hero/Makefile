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

HASHIMYEI_OBJS := $(OUTPUT_PATH)/common/hashimyei_catalog.o \
	$(OUTPUT_PATH)/common/hero_hashimyei_tools.o

# -----------------------------------------------------------------
# Aggregate Target
# -----------------------------------------------------------------
.PHONY: all
all: $(HASHIMYEI_OBJS)
	@$(LOG_SUCCESS)
