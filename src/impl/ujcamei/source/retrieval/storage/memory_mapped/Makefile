ROOT_PATH := ../../../../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/ujcamei/source/retrieval/storage/memory_mapped
REL_MODULE := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))

.DEFAULT_GOAL := all

$(eval $(call BUILD_OBJ, common, memory_mapped_datafile, memory_mapped_datafile))
$(eval $(call BUILD_OBJ_INC, libtorch, memory_mapped_dataset, memory_mapped_dataset, \
  $(TORCH_INCLUDE_PATHS)))
$(eval $(call BUILD_OBJ_INC, libtorch, memory_mapped_dataloader, memory_mapped_dataloader, \
  $(TORCH_INCLUDE_PATHS)))

.PHONY: all
all: memory_mapped_datafile memory_mapped_dataset memory_mapped_dataloader
	@$(LOG_SUCCESS)
