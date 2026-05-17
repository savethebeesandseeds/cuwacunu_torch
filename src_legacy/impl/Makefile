# -----------------------------------------------------------------
# impl/Makefile — recurses into submodules
# -----------------------------------------------------------------
ROOT_PATH := ../
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))
# -----------------------------------------------------------------
.PHONY: iitepi
iitepi:
	$(MAKE) -C $(HERE_PATH)/iitepi all

.PHONY: piaabo
piaabo:
	$(MAKE) -C $(HERE_PATH)/piaabo all

.PHONY: camahjucunu
camahjucunu:
	$(MAKE) -C $(HERE_PATH)/camahjucunu all

.PHONY: wikimyei
wikimyei:
	$(MAKE) -C $(HERE_PATH)/wikimyei all

.PHONY: jkimyei
jkimyei:
	$(MAKE) -C $(HERE_PATH)/jkimyei all

.PHONY: hashimyei
hashimyei:
	$(MAKE) -C $(HERE_PATH)/hero/hashimyei_hero all

.PHONY: lattice_hero
lattice_hero:
	$(MAKE) -C $(HERE_PATH)/hero/lattice_hero all

.PHONY: config_hero
config_hero:
	$(MAKE) -C $(HERE_PATH)/hero/config_hero all

.PHONY: runtime_hero
runtime_hero:
	$(MAKE) -C $(HERE_PATH)/hero/runtime_hero all

.PHONY: marshal_hero
marshal_hero:
	$(MAKE) -C $(HERE_PATH)/hero/marshal_hero all

.PHONY: human_hero
human_hero:
	$(MAKE) -C $(HERE_PATH)/hero/human_hero all

.PHONY: hero
hero: hashimyei lattice_hero config_hero runtime_hero marshal_hero human_hero

.PHONY: all
all: iitepi piaabo camahjucunu wikimyei hashimyei lattice_hero config_hero runtime_hero marshal_hero human_hero
	@$(LOG_SUCCESS)
