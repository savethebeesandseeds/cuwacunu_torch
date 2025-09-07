# -----------------------------------------------------------------
# impl/Makefile — recurses into submodules
# -----------------------------------------------------------------
ROOT_PATH := ..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))
# -----------------------------------------------------------------
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

.PHONY: iinuji
iinuji:
	$(MAKE) -C $(HERE_PATH)/iinuji all

.PHONY: all
all: piaabo camahjucunu wikimyei
	@$(LOG_SUCCESS)
