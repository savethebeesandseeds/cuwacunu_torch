ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config

.DEFAULT_GOAL := all

.PHONY: all latent_lineage_state

latent_lineage_state:
	$(MAKE) -C $(IMPL_PATH)/kikijyeba/lattice/latent_lineage_state all

all: latent_lineage_state
	@$(LOG_SUCCESS)
