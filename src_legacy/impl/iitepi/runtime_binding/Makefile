# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/iitepi/runtime_binding
REL_MODULE := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))

# -----------------------------------------------------------------
# Build Rules
# -----------------------------------------------------------------
$(eval $(call BUILD_OBJ, common/iitepi/runtime_binding, runtime_binding_space_t, \
  iitepi_runtime_binding_space_t))
$(eval $(call BUILD_OBJ, common/iitepi/runtime_binding, runtime_binding_space_t_network_analytics, \
  iitepi_runtime_binding_space_t_network_analytics))

IITEPI_RUNTIME_BINDING := \
  $(OUTPUT_PATH)/common/iitepi/runtime_binding/runtime_binding_space_t.o \
  $(OUTPUT_PATH)/common/iitepi/runtime_binding/runtime_binding_space_t_network_analytics.o

# -----------------------------------------------------------------
# Aggregate Target
# -----------------------------------------------------------------
.PHONY: all
all: $(IITEPI_RUNTIME_BINDING)
	@$(LOG_SUCCESS)
