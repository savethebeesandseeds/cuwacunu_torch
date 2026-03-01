# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/iitepi
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))

# -----------------------------------------------------------------
# Build Rules
# -----------------------------------------------------------------
$(eval $(call BUILD_OBJ, common/iitepi, config_space_t,   iitepi_config_space_t))
$(eval $(call BUILD_OBJ, common/iitepi, contract_space_t, iitepi_contract_space_t))
$(eval $(call BUILD_OBJ, common/iitepi, wave_space_t,     iitepi_wave_space_t))
$(eval $(call BUILD_OBJ, common/iitepi, board_space_t,    iitepi_board_space_t))

IITEPI := \
  $(OUTPUT_PATH)/common/iitepi/config_space_t.o \
  $(OUTPUT_PATH)/common/iitepi/contract_space_t.o \
  $(OUTPUT_PATH)/common/iitepi/wave_space_t.o \
  $(OUTPUT_PATH)/common/iitepi/board_space_t.o

# -----------------------------------------------------------------
# Aggregate Target
# -----------------------------------------------------------------
.PHONY: all
all: $(IITEPI)
	@$(LOG_SUCCESS)
