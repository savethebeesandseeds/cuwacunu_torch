ROOT_PATH := ../..
include $(ROOT_PATH)/Makefile.config

.DEFAULT_GOAL := all

.PHONY: all protocol topology

protocol:
	$(MAKE) -C $(IMPL_PATH)/kikijyeba/protocol all

topology:
	$(MAKE) -C $(IMPL_PATH)/kikijyeba/topology/graph all

all: topology protocol
	@$(LOG_SUCCESS)
