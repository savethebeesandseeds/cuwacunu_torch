# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/camahjucunu/dsl
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))
# -----------------------------------------------------------------
# Aggregate Targets
# -----------------------------------------------------------------
.PHONY: observation_pipeline
observation_pipeline:
	$(MAKE) -C $(HERE_PATH)/observation_pipeline all

.PHONY: latent_lineage_state
latent_lineage_state:
	$(MAKE) -C $(HERE_PATH)/latent_lineage_state all

.PHONY: marshal_objective
marshal_objective:
	$(MAKE) -C $(HERE_PATH)/marshal_objective all

.PHONY: network_design
network_design:
	$(MAKE) -C $(HERE_PATH)/network_design all

.PHONY: jkimyei_specs
jkimyei_specs:
	$(MAKE) -C $(HERE_PATH)/jkimyei_specs all

.PHONY: iitepi_campaign
iitepi_campaign:
	$(MAKE) -C $(HERE_PATH)/iitepi_campaign all

.PHONY: lattice_target
lattice_target:
	$(MAKE) -C $(HERE_PATH)/lattice_target all

.PHONY: tsiemene_circuit
tsiemene_circuit:
	$(MAKE) -C $(HERE_PATH)/tsiemene_circuit all

.PHONY: iitepi_wave
iitepi_wave:
	$(MAKE) -C $(HERE_PATH)/iitepi_wave all

.PHONY: runtime_binding_instruction
runtime_binding_instruction:
	$(MAKE) -C $(HERE_PATH)/runtime_binding_instruction all

.PHONY: canonical_path
canonical_path:
	$(MAKE) -C $(HERE_PATH)/canonical_path all

.PHONY: all
all: observation_pipeline latent_lineage_state marshal_objective network_design jkimyei_specs iitepi_campaign lattice_target tsiemene_circuit iitepi_wave runtime_binding_instruction canonical_path
	@$(LOG_SUCCESS)
