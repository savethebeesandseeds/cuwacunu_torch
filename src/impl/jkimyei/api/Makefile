ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/jkimyei/api
REL_MODULE := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))

.DEFAULT_GOAL := all

$(eval $(call BUILD_OBJ_INC, libtorch, jkimyei_setup, jkimyei_setup, $(TORCH_INCLUDE_PATHS)))

.PHONY: specs
specs:
	@printf "[JKIMYEI:SPECS] public headers and parser implementation live under jkimyei/api\n"

.PHONY: all
all: specs jkimyei_setup
	@$(LOG_SUCCESS)
