# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../../../../
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/camahjucunu/dsl/observation_pipeline
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))
# -----------------------------------------------------------------
# Build Rules
# -----------------------------------------------------------------
$(eval $(call BUILD_OBJ, common, observation_pipeline, dsl_obs_pipe_core))
$(eval $(call BUILD_OBJ, common, sources/observation_sources_pipeline, dsl_obs_pipe_sources))
$(eval $(call BUILD_OBJ, common, channels/observation_channels_pipeline, dsl_obs_pipe_channels))

# -----------------------------------------------------------------
# Aggregate Target
# -----------------------------------------------------------------
.PHONY: all
all: dsl_obs_pipe_core dsl_obs_pipe_sources dsl_obs_pipe_channels
	@$(LOG_SUCCESS)
