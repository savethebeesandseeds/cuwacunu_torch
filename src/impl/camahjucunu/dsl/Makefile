# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/camahjucunu/dsl
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))
# -----------------------------------------------------------------
# Aggregate Target
# -----------------------------------------------------------------
.PHONY: all
all:
	$(MAKE) -C $(HERE_PATH)/observation_pipeline all
	$(MAKE) -C $(HERE_PATH)/latent_lineage_state all
	$(MAKE) -C $(HERE_PATH)/network_design all
	$(MAKE) -C $(HERE_PATH)/jkimyei_specs all
	$(MAKE) -C $(HERE_PATH)/iitepi_campaign all
	$(MAKE) -C $(HERE_PATH)/tsiemene_circuit all
	$(MAKE) -C $(HERE_PATH)/iitepi_wave all
	$(MAKE) -C $(HERE_PATH)/runtime_binding_instruction all
	$(MAKE) -C $(HERE_PATH)/canonical_path all
	@$(LOG_SUCCESS)
