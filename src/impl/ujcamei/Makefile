ROOT_PATH := ../..
include $(ROOT_PATH)/Makefile.config

.DEFAULT_GOAL := all

.PHONY: all source

all:
	@printf "[UJCAMEI] implementation modules migrated; build waits for namespace/config pass\n"

source:
	$(MAKE) -C $(IMPL_PATH)/ujcamei/source all
