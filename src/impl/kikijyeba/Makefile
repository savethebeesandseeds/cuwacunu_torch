ROOT_PATH := ../..
include $(ROOT_PATH)/Makefile.config

.DEFAULT_GOAL := all

.PHONY: all protocol settings topology lattice

protocol:
	$(MAKE) -C $(IMPL_PATH)/kikijyeba/protocol all

settings:
	$(MAKE) -C $(IMPL_PATH)/kikijyeba/settings all

topology:
	$(MAKE) -C $(IMPL_PATH)/kikijyeba/topology/graph all

lattice:
	$(MAKE) -C $(IMPL_PATH)/kikijyeba/lattice all

all: topology protocol settings lattice
	@$(LOG_SUCCESS)
