# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/piaabo/torch_compat
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))
# -----------------------------------------------------------------
# Build Rules (producer; no lib*.a prerequisites)
#   - Use BUILD_OBJ_INC to add Torch headers
#   - Produces: build/libtorch/torch_utils.o, build/libtorch/network_analytics.o,
#               build/libtorch/network_design_adapter.o, build/libtorch/data_analytics.o,
#               build/libtorch/entropic_capacity_comparison.o
#   - Exposes:  torch_utils / network_analytics / network_design_adapter /
#               data_analytics / entropic_capacity_comparison as named goals
# -----------------------------------------------------------------
$(eval $(call BUILD_OBJ_INC, libtorch, torch_utils, torch_utils, $(TORCH_INCLUDE_PATHS)))
$(eval $(call BUILD_OBJ_INC, libtorch, network_analytics, network_analytics, $(TORCH_INCLUDE_PATHS)))
$(eval $(call BUILD_OBJ_INC, libtorch, network_design_adapter, network_design_adapter, $(TORCH_INCLUDE_PATHS)))
$(eval $(call BUILD_OBJ_INC, libtorch, data_analytics, data_analytics, $(TORCH_INCLUDE_PATHS)))
$(eval $(call BUILD_OBJ_INC, libtorch, entropic_capacity_comparison, entropic_capacity_comparison, $(TORCH_INCLUDE_PATHS)))

NETWORK_ANALYTICS_PARTS := \
  $(HERE_PATH)/network_analytics_private_accumulators.cpp \
  $(HERE_PATH)/network_analytics_private_module.cpp \
  $(HERE_PATH)/network_analytics_private_design.cpp \
  $(HERE_PATH)/network_analytics_public.cpp

DATA_ANALYTICS_PARTS := \
  $(HERE_PATH)/data_analytics_private_helpers.cpp \
  $(HERE_PATH)/data_analytics_private_accumulators.cpp \
  $(HERE_PATH)/data_analytics_private_reports.cpp \
  $(HERE_PATH)/data_analytics_public.cpp

$(OUTPUT_PATH)/libtorch/network_analytics.o: $(NETWORK_ANALYTICS_PARTS)
$(OUTPUT_PATH)/libtorch/data_analytics.o: $(DATA_ANALYTICS_PARTS)

# -----------------------------------------------------------------
# Aggregate Targets
#   - Expose distributions as a sibling prerequisite so GNU make can schedule
#     it in parallel with the local torch_compat producers.
# -----------------------------------------------------------------
.PHONY: distributions
distributions:
	$(MAKE) -C $(HERE_PATH)/distributions all

.PHONY: all
all: torch_utils network_analytics network_design_adapter data_analytics entropic_capacity_comparison distributions
	@$(LOG_SUCCESS)
