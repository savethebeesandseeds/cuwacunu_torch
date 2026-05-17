ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/wikimyei/expression/nodelift
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))

.PHONY: srl
srl:
	$(MAKE) -C $(HERE_PATH)/srl all

.PHONY: all
all: srl
	@$(LOG_SUCCESS)
