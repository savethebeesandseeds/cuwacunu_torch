# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/hero/config_hero
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))

# -----------------------------------------------------------------
# Build Rules
# -----------------------------------------------------------------
$(eval $(call BUILD_OBJ_INC, common, hero_config_store, hero_config_store))
$(eval $(call BUILD_OBJ_INC, common, hero_config_tools, hero_config_tools))

CONFIG_HERO_OBJS := \
  $(OUTPUT_PATH)/common/hero_config_store.o \
  $(OUTPUT_PATH)/common/hero_config_tools.o

# -----------------------------------------------------------------
# Aggregate Target
# -----------------------------------------------------------------
.PHONY: all
all: $(CONFIG_HERO_OBJS)
	@$(LOG_SUCCESS)
