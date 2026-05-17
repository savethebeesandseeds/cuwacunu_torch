ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config

.DEFAULT_GOAL := all

.PHONY: all runtime_report

runtime_report:
	$(MAKE) -C $(IMPL_PATH)/kikijyeba/lattice/latent_lineage_state/runtime_report all

all: runtime_report
	@$(LOG_SUCCESS)
