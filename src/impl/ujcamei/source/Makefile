ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config

.DEFAULT_GOAL := all

.PHONY: all contract types storage dataloader

all: contract types storage dataloader
	@printf "[UJCAMEI:SOURCE] migrated; source-universe runtime active\n"

contract:
	$(MAKE) -C $(IMPL_PATH)/ujcamei/source/contract all

types:
	$(MAKE) -C $(IMPL_PATH)/ujcamei/source/types all

storage:
	$(MAKE) -C $(IMPL_PATH)/ujcamei/source/storage all

dataloader:
	$(MAKE) -C $(IMPL_PATH)/ujcamei/source/dataloader all
