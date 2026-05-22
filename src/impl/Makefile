ROOT_PATH := ..
include $(ROOT_PATH)/Makefile.config

.DEFAULT_GOAL := all

.PHONY: all piaabo wikimyei ujcamei jkimyei kikijyeba hero

piaabo:
	$(MAKE) -C $(IMPL_PATH)/piaabo all

wikimyei:
	$(MAKE) -C $(IMPL_PATH)/wikimyei all

ujcamei:
	$(MAKE) -C $(IMPL_PATH)/ujcamei all

jkimyei:
	$(MAKE) -C $(IMPL_PATH)/jkimyei all

kikijyeba:
	$(MAKE) -C $(IMPL_PATH)/kikijyeba all

hero:
	$(MAKE) -C $(IMPL_PATH)/hero all

all: piaabo wikimyei ujcamei jkimyei kikijyeba hero
