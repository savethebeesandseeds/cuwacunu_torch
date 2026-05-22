ROOT_PATH := ../../../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/ujcamei/source/registry/types
REL_MODULE := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))

.DEFAULT_GOAL := all

$(eval $(call BUILD_OBJ, common, registry_data, registry_data))
$(eval $(call BUILD_OBJ, common, registry_utils, registry_utils))

.PHONY: all
all: registry_data registry_utils
	@$(LOG_SUCCESS)
