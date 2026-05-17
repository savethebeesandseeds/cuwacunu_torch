ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/piaabo/tensor
REL_MODULE := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))

.DEFAULT_GOAL := all

.PHONY: network_design
network_design:
	$(MAKE) -C $(HERE_PATH)/network_design all

.PHONY: torch
torch:
	$(MAKE) -C $(HERE_PATH)/torch all

.PHONY: all
all: network_design torch
	@$(LOG_SUCCESS)
