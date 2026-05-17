# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/hero/human_hero
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))

# -----------------------------------------------------------------
# Build Rules
# -----------------------------------------------------------------
$(eval $(call BUILD_OBJ_INC, common, human_attestation, human_attestation, $(SSL_INCLUDE_PATHS)))
$(eval $(call BUILD_OBJ_INC, common, hero_human_tools, hero_human_tools, $(NCURSES_INCLUDE_PATHS)))

HERO_HUMAN_TOOLS_PARTS := \
  $(HERE_PATH)/hero_human_tools_private_core.cpp \
  $(HERE_PATH)/hero_human_tools_private_ncurses.cpp \
  $(HERE_PATH)/hero_human_tools_private_ncurses_draw.cpp \
  $(HERE_PATH)/hero_human_tools_private_ncurses_dialogs.cpp \
  $(HERE_PATH)/hero_human_tools_private_ncurses_render.cpp \
  $(HERE_PATH)/hero_human_tools_private_ncurses_console.cpp \
  $(HERE_PATH)/hero_human_tools_private_ops.cpp \
  $(HERE_PATH)/hero_human_tools_private_handlers.cpp \
  $(HERE_PATH)/hero_human_tools_public.cpp

$(OUTPUT_PATH)/common/hero_human_tools.o: $(HERO_HUMAN_TOOLS_PARTS)

HUMAN_HERO_TARGETS := \
  human_attestation \
  hero_human_tools

# -----------------------------------------------------------------
# Aggregate Target
# -----------------------------------------------------------------
.PHONY: all
all: $(HUMAN_HERO_TARGETS)
	@$(LOG_SUCCESS)
