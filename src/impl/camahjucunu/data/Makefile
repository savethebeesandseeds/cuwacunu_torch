# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/camahjucunu/data
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))
# -----------------------------------------------------------------
# Build Rules (producers; no lib*.a prerequisites)
#   - Use BUILD_OBJ_INC to include Torch headers for these objects
#   - Produces: build/libtorch/*.o
# -----------------------------------------------------------------
$(eval $(call BUILD_OBJ_INC, libtorch, memory_mapped_datafile,   mm_datafile,  $(TORCH_INCLUDE_PATHS)))
$(eval $(call BUILD_OBJ_INC, libtorch, memory_mapped_dataset,    mm_dataset,   $(TORCH_INCLUDE_PATHS)))
$(eval $(call BUILD_OBJ_INC, libtorch, memory_mapped_dataloader, mm_dataloader,$(TORCH_INCLUDE_PATHS)))

# Group the leaf producers for a convenient target
DATALOADER_DATASETS := \
  mm_datafile \
  mm_dataset \
  mm_dataloader

# -----------------------------------------------------------------
# Aggregate Target (module-level)
# -----------------------------------------------------------------
.PHONY: types
types:
	$(MAKE) -C $(HERE_PATH)/types all
	@$(LOG_SUCCESS)

.PHONY: datasets
datasets: $(DATALOADER_DATASETS)
	@$(LOG_SUCCESS)

# -----------------------------------------------------------------
# All
# -----------------------------------------------------------------
.PHONY: all
all: types datasets
	@$(LOG_SUCCESS)
