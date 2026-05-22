ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/kikijyeba/lattice
REL_MODULE := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))

.DEFAULT_GOAL := all

$(eval $(call BUILD_OBJ, common, lattice, lattice))

.PHONY: all runtime_report

runtime_report:
	$(MAKE) -C $(IMPL_PATH)/kikijyeba/lattice/runtime_report all

all: lattice runtime_report
	@$(LOG_SUCCESS)
