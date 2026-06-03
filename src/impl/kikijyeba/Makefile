ROOT_PATH := ../..
include $(ROOT_PATH)/Makefile.config

.DEFAULT_GOAL := all

.PHONY: all protocol topology lattice

protocol:
	$(MAKE) -C $(IMPL_PATH)/kikijyeba/protocol all

topology:
	$(MAKE) -C $(IMPL_PATH)/kikijyeba/topology/graph all

lattice:
	$(MAKE) -C $(IMPL_PATH)/kikijyeba/lattice all

all: topology protocol lattice
	@$(LOG_SUCCESS)
