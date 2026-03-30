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
$(eval $(call BUILD_OBJ_INC, common, hero_config_tools_config_policy, hero_config_tools_config_policy))
$(eval $(call BUILD_OBJ_INC, common, hero_config_tools_config_file_ops, hero_config_tools_config_file_ops))
$(eval $(call BUILD_OBJ_INC, common, hero_config_tools_optim_ops, hero_config_tools_optim_ops))
$(eval $(call BUILD_OBJ_INC, common, hero_config_tools_dev_reset_ops, hero_config_tools_dev_reset_ops))

CONFIG_HERO_OBJS := \
  $(OUTPUT_PATH)/common/hero_config_store.o \
  $(OUTPUT_PATH)/common/hero_config_tools.o \
  $(OUTPUT_PATH)/common/hero_config_tools_config_policy.o \
  $(OUTPUT_PATH)/common/hero_config_tools_config_file_ops.o \
  $(OUTPUT_PATH)/common/hero_config_tools_optim_ops.o \
  $(OUTPUT_PATH)/common/hero_config_tools_dev_reset_ops.o

# -----------------------------------------------------------------
# Aggregate Target
# -----------------------------------------------------------------
.PHONY: all
all: $(CONFIG_HERO_OBJS)
	@$(LOG_SUCCESS)
