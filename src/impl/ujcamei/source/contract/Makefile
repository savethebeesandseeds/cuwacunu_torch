ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/ujcamei/source/contract
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

$(OUTPUT_PATH)/common/source_forms_decoder.o: \
	$(HERE_PATH)/dsl/source_forms_decoder.cpp \
	$(HERE_PATH)/Makefile \
	$(SRC_ROOT)/Makefile.config
	$(CC_RULE)

$(OUTPUT_PATH)/common/channel_forms_decoder.o: \
	$(HERE_PATH)/dsl/channel_forms_decoder.cpp \
	$(HERE_PATH)/Makefile \
	$(SRC_ROOT)/Makefile.config
	$(CC_RULE)

$(OUTPUT_PATH)/common/graph_forms_decoder.o: \
	$(HERE_PATH)/dsl/graph_forms_decoder.cpp \
	$(HERE_PATH)/Makefile \
	$(SRC_ROOT)/Makefile.config
	$(CC_RULE)

.PHONY: contract_model runtime_decode channel_forms_decoder source_forms_decoder graph_forms_decoder
contract_model: $(OUTPUT_PATH)/common/source_contract.o
	@printf "$(COLOR_DIM_YELLOW)[$(COLOR_GRAY)MODULE: $(COLOR_DIM_GREEN)OK$(COLOR_DIM_YELLOW)]$(COLOR_RESET)  $(COLOR_GREEN)%s/contract$(COLOR_RESET)\n" "$(REL_MODULE)"

runtime_decode: $(OUTPUT_PATH)/common/source_contract_decode.o
	@printf "$(COLOR_DIM_YELLOW)[$(COLOR_GRAY)MODULE: $(COLOR_DIM_GREEN)OK$(COLOR_DIM_YELLOW)]$(COLOR_RESET)  $(COLOR_GREEN)%s/runtime/decode$(COLOR_RESET)\n" "$(REL_MODULE)"

channel_forms_decoder: $(OUTPUT_PATH)/common/channel_forms_decoder.o
	@printf "$(COLOR_DIM_YELLOW)[$(COLOR_GRAY)MODULE: $(COLOR_DIM_GREEN)OK$(COLOR_DIM_YELLOW)]$(COLOR_RESET)  $(COLOR_GREEN)%s/dsl/channel_forms_decoder$(COLOR_RESET)\n" "$(REL_MODULE)"

source_forms_decoder: $(OUTPUT_PATH)/common/source_forms_decoder.o
	@printf "$(COLOR_DIM_YELLOW)[$(COLOR_GRAY)MODULE: $(COLOR_DIM_GREEN)OK$(COLOR_DIM_YELLOW)]$(COLOR_RESET)  $(COLOR_GREEN)%s/dsl/source_forms_decoder$(COLOR_RESET)\n" "$(REL_MODULE)"

graph_forms_decoder: $(OUTPUT_PATH)/common/graph_forms_decoder.o
	@printf "$(COLOR_DIM_YELLOW)[$(COLOR_GRAY)MODULE: $(COLOR_DIM_GREEN)OK$(COLOR_DIM_YELLOW)]$(COLOR_RESET)  $(COLOR_GREEN)%s/dsl/graph_forms_decoder$(COLOR_RESET)\n" "$(REL_MODULE)"

.PHONY: all
all: contract_model runtime_decode channel_forms_decoder source_forms_decoder graph_forms_decoder
	@$(LOG_SUCCESS)
