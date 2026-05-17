ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/ujcamei/source/types
REL_MODULE := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))

.DEFAULT_GOAL := all

$(eval $(call BUILD_OBJ, common, types_data, types_data))
$(eval $(call BUILD_OBJ, common, types_utils, types_utils))

.PHONY: all
all: types_data types_utils
	@$(LOG_SUCCESS)
