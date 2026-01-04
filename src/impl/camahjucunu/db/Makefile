# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/camahjucunu/db
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))
# -----------------------------------------------------------------
# Build Rules (producers; no lib*.a prerequisites)
#   - Use BUILD_OBJ_INC
#   - Produces in: build/common/*.o
# -----------------------------------------------------------------
$(eval $(call BUILD_OBJ_INC, common, db,   db))

# Group the objects for a convenient target
DATALOADER_DATASETS := \
  $(OUTPUT_PATH)/common/db.o

# -----------------------------------------------------------------
# Aggregate Target (module-level)
# -----------------------------------------------------------------
.PHONY: datasets
datasets: $(DATALOADER_DATASETS)
	@$(LOG_SUCCESS)

# -----------------------------------------------------------------
# All
# -----------------------------------------------------------------
.PHONY: all
all: datasets
	@$(LOG_SUCCESS)
