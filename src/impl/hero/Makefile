ROOT_PATH := ../..
include $(ROOT_PATH)/Makefile.config

.DEFAULT_GOAL := all

.PHONY: all config_hero runtime_hero lattice_hero

config_hero:
	$(MAKE) -C $(IMPL_PATH)/hero/config_hero all

runtime_hero:
	$(MAKE) -C $(IMPL_PATH)/hero/runtime_hero all

lattice_hero:
	$(MAKE) -C $(IMPL_PATH)/hero/lattice_hero all

all: config_hero runtime_hero lattice_hero
	@$(LOG_SUCCESS)
