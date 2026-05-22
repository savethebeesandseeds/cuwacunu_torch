ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/kikijyeba/lattice/runtime_report
REL_MODULE := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))

$(eval $(call BUILD_OBJ, common, runtime_lls, runtime_lls))

.PHONY: all
all: runtime_lls
	@$(LOG_SUCCESS)
