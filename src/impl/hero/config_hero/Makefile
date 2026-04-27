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

HERO_CONFIG_TOOLS_PARTS := \
  $(HERE_PATH)/hero_config_tools_private_io_json.cpp \
  $(HERE_PATH)/hero_config_tools_private_results.cpp \
  $(HERE_PATH)/hero_config_tools_private_dispatch.cpp \
  $(HERE_PATH)/hero_config_tools_public.cpp

HERO_CONFIG_TOOLS_CONFIG_FILE_OPS_PARTS := \
  $(HERE_PATH)/hero_config_tools_config_file_ops_private_validation.cpp \
  $(HERE_PATH)/hero_config_tools_config_file_ops_private_default_temp.cpp \
  $(HERE_PATH)/hero_config_tools_config_file_ops_private_objective.cpp

HERO_CONFIG_TOOLS_OPTIM_OPS_PARTS := \
  $(HERE_PATH)/hero_config_tools_optim_ops_private_surface.cpp \
  $(HERE_PATH)/hero_config_tools_optim_ops_private_policy.cpp \
  $(HERE_PATH)/hero_config_tools_optim_ops_private_read_list.cpp \
  $(HERE_PATH)/hero_config_tools_optim_ops_private_mutation.cpp \
  $(HERE_PATH)/hero_config_tools_optim_ops_private_backups.cpp

$(OUTPUT_PATH)/common/hero_config_tools.o: $(HERO_CONFIG_TOOLS_PARTS)
$(OUTPUT_PATH)/common/hero_config_tools_config_file_ops.o: $(HERO_CONFIG_TOOLS_CONFIG_FILE_OPS_PARTS)
$(OUTPUT_PATH)/common/hero_config_tools_optim_ops.o: $(HERO_CONFIG_TOOLS_OPTIM_OPS_PARTS)

CONFIG_HERO_TARGETS := \
  hero_config_store \
  hero_config_tools \
  hero_config_tools_config_policy \
  hero_config_tools_config_file_ops \
  hero_config_tools_optim_ops \
  hero_config_tools_dev_reset_ops

# -----------------------------------------------------------------
# Aggregate Target
# -----------------------------------------------------------------
.PHONY: all
all: $(CONFIG_HERO_TARGETS)
	@$(LOG_SUCCESS)
