ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/ujcamei/source/contract
SOURCE_IMPL_PATH := $(IMPL_PATH)/ujcamei/source
KIKIJYEBA_TOPOLOGY_IMPL_PATH := $(IMPL_PATH)/kikijyeba/topology/graph
REL_MODULE := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))

.DEFAULT_GOAL := all

$(OUTPUT_PATH)/common/source_contract.o: \
	$(HERE_PATH)/contract.cpp \
	$(HERE_PATH)/Makefile \
	$(SRC_ROOT)/Makefile.config
	$(CC_RULE)

$(OUTPUT_PATH)/common/source_contract_decode.o: \
	$(HERE_PATH)/runtime/decode.cpp \
	$(HERE_PATH)/Makefile \
	$(SRC_ROOT)/Makefile.config
	$(CC_RULE)

$(OUTPUT_PATH)/common/source_registry_decoder.o: \
	$(SOURCE_IMPL_PATH)/registry/source_registry_decoder.cpp \
	$(HERE_PATH)/Makefile \
	$(SRC_ROOT)/Makefile.config
	$(CC_RULE)

$(OUTPUT_PATH)/common/retrieval_channel_decoder.o: \
	$(SOURCE_IMPL_PATH)/retrieval/retrieval_channel_decoder.cpp \
	$(HERE_PATH)/Makefile \
	$(SRC_ROOT)/Makefile.config
	$(CC_RULE)

$(OUTPUT_PATH)/common/graph_topology_decoder.o: \
	$(KIKIJYEBA_TOPOLOGY_IMPL_PATH)/graph_topology_decoder.cpp \
	$(HERE_PATH)/Makefile \
	$(SRC_ROOT)/Makefile.config
	$(CC_RULE)

.PHONY: contract_model runtime_decode retrieval_channel_decoder source_registry_decoder graph_topology_decoder
contract_model: $(OUTPUT_PATH)/common/source_contract.o
	@printf "$(COLOR_DIM_YELLOW)[$(COLOR_GRAY)MODULE: $(COLOR_DIM_GREEN)OK$(COLOR_DIM_YELLOW)]$(COLOR_RESET)  $(COLOR_GREEN)%s/contract$(COLOR_RESET)\n" "$(REL_MODULE)"

runtime_decode: $(OUTPUT_PATH)/common/source_contract_decode.o
	@printf "$(COLOR_DIM_YELLOW)[$(COLOR_GRAY)MODULE: $(COLOR_DIM_GREEN)OK$(COLOR_DIM_YELLOW)]$(COLOR_RESET)  $(COLOR_GREEN)%s/runtime/decode$(COLOR_RESET)\n" "$(REL_MODULE)"

retrieval_channel_decoder: $(OUTPUT_PATH)/common/retrieval_channel_decoder.o
	@printf "$(COLOR_DIM_YELLOW)[$(COLOR_GRAY)MODULE: $(COLOR_DIM_GREEN)OK$(COLOR_DIM_YELLOW)]$(COLOR_RESET)  $(COLOR_GREEN)%s/retrieval/retrieval_channel_decoder$(COLOR_RESET)\n" "ujcamei/source"

source_registry_decoder: $(OUTPUT_PATH)/common/source_registry_decoder.o
	@printf "$(COLOR_DIM_YELLOW)[$(COLOR_GRAY)MODULE: $(COLOR_DIM_GREEN)OK$(COLOR_DIM_YELLOW)]$(COLOR_RESET)  $(COLOR_GREEN)%s/registry/source_registry_decoder$(COLOR_RESET)\n" "ujcamei/source"

graph_topology_decoder: $(OUTPUT_PATH)/common/graph_topology_decoder.o
	@printf "$(COLOR_DIM_YELLOW)[$(COLOR_GRAY)MODULE: $(COLOR_DIM_GREEN)OK$(COLOR_DIM_YELLOW)]$(COLOR_RESET)  $(COLOR_GREEN)%s/graph_topology_decoder$(COLOR_RESET)\n" "kikijyeba/topology/graph"

.PHONY: all
all: contract_model runtime_decode retrieval_channel_decoder source_registry_decoder graph_topology_decoder
	@$(LOG_SUCCESS)
