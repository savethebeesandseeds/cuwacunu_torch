ROOT_PATH := ../../../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/wikimyei/expression/nodelift/srl
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))

$(eval $(call BUILD_OBJ_INC, libtorch, synthetic_reference_lift, synthetic_reference_lift, $(TORCH_INCLUDE_PATHS)))
$(eval $(call BUILD_OBJ_INC, libtorch, stream/srl_graph_adapter, srl_graph_adapter, $(TORCH_INCLUDE_PATHS)))

.PHONY: all
all: synthetic_reference_lift srl_graph_adapter
	@$(LOG_SUCCESS)
