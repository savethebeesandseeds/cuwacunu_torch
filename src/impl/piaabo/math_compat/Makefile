# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/piaabo/math_compat
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))
# -----------------------------------------------------------------
# Build Rules (producer; no lib*.a prerequisites)
#   - BUILD_OBJ(module, source_basename, phony_target)
#   - Produces: build/common/statistics_space.o
#   - Exposes:  statistics as a named goal
# -----------------------------------------------------------------
$(eval $(call BUILD_OBJ, common, statistics_space, statistics))

# -----------------------------------------------------------------
# Aggregate Target
# -----------------------------------------------------------------
.PHONY: all
all: statistics
	@$(LOG_SUCCESS)
