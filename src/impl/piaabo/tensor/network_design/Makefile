ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/piaabo/tensor/network_design
REL_MODULE := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))

.DEFAULT_GOAL := all

$(eval $(call BUILD_OBJ, common, network_design, network_design))

.PHONY: all

all: network_design
	@$(LOG_SUCCESS)
