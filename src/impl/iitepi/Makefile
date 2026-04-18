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
$(eval $(call BUILD_OBJ, common/iitepi, contract_space_t_network_analytics, iitepi_contract_space_t_network_analytics))
$(eval $(call BUILD_OBJ, common/iitepi, wave_space_t,     iitepi_wave_space_t))
IITEPI_HERE_PATH := $(HERE_PATH)
IITEPI_REL_MODULE := $(REL_MODULE)
HERE_PATH := $(IMPL_PATH)/iitepi/runtime_binding
REL_MODULE := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
$(eval $(call BUILD_OBJ, common/iitepi/runtime_binding, runtime_binding_space_t, \
  iitepi_runtime_binding_space_t))
$(eval $(call BUILD_OBJ, common/iitepi/runtime_binding, runtime_binding_space_t_network_analytics, \
  iitepi_runtime_binding_space_t_network_analytics))
HERE_PATH := $(IITEPI_HERE_PATH)
REL_MODULE := $(IITEPI_REL_MODULE)

IITEPI := \
  iitepi_config_space_t \
  iitepi_contract_space_t \
  iitepi_contract_space_t_network_analytics \
  iitepi_wave_space_t \
  iitepi_runtime_binding_space_t \
  iitepi_runtime_binding_space_t_network_analytics

# -----------------------------------------------------------------
# Aggregate Target
# -----------------------------------------------------------------
.PHONY: all
all: $(IITEPI)
	@$(LOG_SUCCESS)
