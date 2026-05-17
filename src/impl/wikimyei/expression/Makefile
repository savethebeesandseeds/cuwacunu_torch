ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/wikimyei/expression
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))

.DEFAULT_GOAL := all

.PHONY: nodelift
nodelift:
	$(MAKE) -C $(HERE_PATH)/nodelift all

.PHONY: all
all: nodelift
	@$(LOG_SUCCESS)
