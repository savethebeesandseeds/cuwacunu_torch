# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/hero/runtime_hero
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))

# -----------------------------------------------------------------
# Build Rules
# -----------------------------------------------------------------
$(eval $(call BUILD_OBJ_INC, common, hero_runtime_tools, hero_runtime_tools))

HERO_RUNTIME_TOOLS_PARTS := \
  $(HERE_PATH)/hero_runtime_tools_private_core.cpp \
  $(HERE_PATH)/hero_runtime_tools_private_handlers.cpp \
  $(HERE_PATH)/hero_runtime_tools_private_public.cpp

$(OUTPUT_PATH)/common/hero_runtime_tools.o: $(HERO_RUNTIME_TOOLS_PARTS)

RUNTIME_HERO_TARGETS := \
  hero_runtime_tools

# -----------------------------------------------------------------
# Aggregate Target
# -----------------------------------------------------------------
.PHONY: all
all: $(RUNTIME_HERO_TARGETS)
	@$(LOG_SUCCESS)
