ROOT_PATH := ../..
include $(ROOT_PATH)/Makefile.config

.DEFAULT_GOAL := all

.PHONY: all api specs training evaluation

api:
	$(MAKE) -C $(IMPL_PATH)/jkimyei/api all

specs:
	$(MAKE) -C $(IMPL_PATH)/jkimyei/api specs

training:
	$(MAKE) -C $(IMPL_PATH)/jkimyei/training all

evaluation:
	$(MAKE) -C $(IMPL_PATH)/jkimyei/evaluation all

all:
	@printf "[JKIMYEI] implementation rooms migrated; build waits for namespace/bridge pass\n"
