ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/kikijyeba/topology/graph
REL_MODULE := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))

.DEFAULT_GOAL := all

$(OUTPUT_PATH)/common/graph_topology_decoder.o: \
	$(HERE_PATH)/graph_topology_decoder.cpp \
	$(HERE_PATH)/Makefile \
	$(SRC_ROOT)/Makefile.config
	$(CC_RULE)

.PHONY: all graph_topology_decoder

graph_topology_decoder: $(OUTPUT_PATH)/common/graph_topology_decoder.o
	@printf "$(COLOR_DIM_YELLOW)[$(COLOR_GRAY)MODULE: $(COLOR_DIM_GREEN)OK$(COLOR_DIM_YELLOW)]$(COLOR_RESET)  $(COLOR_GREEN)%s/graph_topology_decoder$(COLOR_RESET)\n" "$(REL_MODULE)"

all: graph_topology_decoder
	@$(LOG_SUCCESS)
