# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/hero/super_hero
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))

# -----------------------------------------------------------------
# Build Rules
# -----------------------------------------------------------------
$(eval $(call BUILD_OBJ_INC, common, hero_super_tools, hero_super_tools))
$(eval $(call BUILD_OBJ_INC, common, super_loop_workspace, super_loop_workspace))

SUPER_HERO_OBJS := \
  $(OUTPUT_PATH)/common/hero_super_tools.o \
  $(OUTPUT_PATH)/common/super_loop_workspace.o

# -----------------------------------------------------------------
# Aggregate Target
# -----------------------------------------------------------------
.PHONY: all
all: $(SUPER_HERO_OBJS)
	@$(LOG_SUCCESS)
