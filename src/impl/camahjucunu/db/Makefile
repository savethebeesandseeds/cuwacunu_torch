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
$(eval $(call BUILD_OBJ_INC, common, idydb, idydb))

# Group the leaf producer for a convenient target
DB_DATASETS := \
  idydb

# -----------------------------------------------------------------
# Aggregate Target (module-level)
# -----------------------------------------------------------------
.PHONY: datasets
datasets: $(DB_DATASETS)
	@$(LOG_SUCCESS)

# -----------------------------------------------------------------
# All
# -----------------------------------------------------------------
.PHONY: all
all: datasets
	@$(LOG_SUCCESS)
