# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/hero/marshal_hero
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))

# -----------------------------------------------------------------
# Build Rules
# -----------------------------------------------------------------
$(eval $(call BUILD_OBJ_INC, common, hero_marshal_tools, hero_marshal_tools))
$(eval $(call BUILD_OBJ_INC, common, marshal_session_workspace, marshal_session_workspace))

MARSHAL_HERO_OBJS := \
  $(OUTPUT_PATH)/common/hero_marshal_tools.o \
  $(OUTPUT_PATH)/common/marshal_session_workspace.o

# -----------------------------------------------------------------
# Aggregate Target
# -----------------------------------------------------------------
.PHONY: all
all: $(MARSHAL_HERO_OBJS)
	@$(LOG_SUCCESS)
