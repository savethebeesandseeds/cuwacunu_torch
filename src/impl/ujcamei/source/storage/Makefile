ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config

.DEFAULT_GOAL := all

.PHONY: all memory_mapped

memory_mapped:
	$(MAKE) -C $(IMPL_PATH)/ujcamei/source/storage/memory_mapped all

all: memory_mapped
	@$(LOG_SUCCESS)
