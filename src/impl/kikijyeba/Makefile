ROOT_PATH := ../..
include $(ROOT_PATH)/Makefile.config

.DEFAULT_GOAL := all

.PHONY: all composition lattice

composition:
	$(MAKE) -C $(IMPL_PATH)/kikijyeba/composition all

lattice:
	$(MAKE) -C $(IMPL_PATH)/kikijyeba/lattice all

all: composition lattice
	@$(LOG_SUCCESS)
