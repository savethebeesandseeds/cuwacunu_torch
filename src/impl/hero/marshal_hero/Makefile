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

HERO_MARSHAL_TOOLS_PARTS := \
  $(HERE_PATH)/hero_marshal_tools_private_core.cpp \
  $(HERE_PATH)/hero_marshal_tools_private_session.cpp \
  $(HERE_PATH)/hero_marshal_tools_private_objective.cpp \
  $(HERE_PATH)/hero_marshal_tools_private_checkpoints.cpp \
  $(HERE_PATH)/hero_marshal_tools_private_runner.cpp \
  $(HERE_PATH)/hero_marshal_tools_private_handlers.cpp \
  $(HERE_PATH)/hero_marshal_tools_public.cpp

$(OUTPUT_PATH)/common/hero_marshal_tools.o: $(HERO_MARSHAL_TOOLS_PARTS)

MARSHAL_HERO_TARGETS := \
  hero_marshal_tools \
  marshal_session_workspace

# -----------------------------------------------------------------
# Aggregate Target
# -----------------------------------------------------------------
.PHONY: all
all: $(MARSHAL_HERO_TARGETS)
	@$(LOG_SUCCESS)
