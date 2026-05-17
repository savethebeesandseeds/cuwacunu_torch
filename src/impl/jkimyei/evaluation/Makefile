ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/jkimyei/evaluation
REL_MODULE := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))

$(eval $(call BUILD_OBJ_INC, libtorch, representation/network_analytics, network_analytics, $(TORCH_INCLUDE_PATHS)))
$(eval $(call BUILD_OBJ_INC, libtorch, source/data_analytics, data_analytics, $(TORCH_INCLUDE_PATHS)))
$(eval $(call BUILD_OBJ_INC, libtorch, inference/entropic_capacity_comparison, entropic_capacity_comparison, $(TORCH_INCLUDE_PATHS)))

NETWORK_ANALYTICS_PARTS := \
  $(HERE_PATH)/representation/network_analytics_private_accumulators.cpp \
  $(HERE_PATH)/representation/network_analytics_private_module.cpp \
  $(HERE_PATH)/representation/network_analytics_private_design.cpp \
  $(HERE_PATH)/representation/network_analytics_public.cpp

DATA_ANALYTICS_PARTS := \
  $(HERE_PATH)/source/data_analytics_private_helpers.cpp \
  $(HERE_PATH)/source/data_analytics_private_accumulators.cpp \
  $(HERE_PATH)/source/data_analytics_private_reports.cpp \
  $(HERE_PATH)/source/data_analytics_public.cpp

$(OUTPUT_PATH)/libtorch/representation/network_analytics.o: $(NETWORK_ANALYTICS_PARTS)
$(OUTPUT_PATH)/libtorch/source/data_analytics.o: $(DATA_ANALYTICS_PARTS)

JKIMYEI_EVALUATION := \
  network_analytics \
  data_analytics \
  entropic_capacity_comparison

.PHONY: all
all: $(JKIMYEI_EVALUATION)
	@$(LOG_SUCCESS)
