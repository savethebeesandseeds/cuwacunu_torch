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

HUMAN_HERO_OBJS := \
  $(OUTPUT_PATH)/common/human_attestation.o \
  $(OUTPUT_PATH)/common/hero_human_tools.o

# -----------------------------------------------------------------
# Aggregate Target
# -----------------------------------------------------------------
.PHONY: all
all: $(HUMAN_HERO_OBJS)
	@$(LOG_SUCCESS)
