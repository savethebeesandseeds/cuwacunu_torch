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

HUMAN_HERO_TARGETS := \
  human_attestation \
  hero_human_tools

# -----------------------------------------------------------------
# Aggregate Target
# -----------------------------------------------------------------
.PHONY: all
all: $(HUMAN_HERO_TARGETS)
	@$(LOG_SUCCESS)
