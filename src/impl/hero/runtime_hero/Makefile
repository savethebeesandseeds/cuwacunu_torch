ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config

.DEFAULT_GOAL := all

$(OUTPUT_PATH)/common/hero_runtime_tools.o: \
	$(IMPL_PATH)/hero/runtime_hero/hero_runtime_tools.cpp \
	$(SRC_ROOT)/Makefile.config
	$(CC_RULE)

.PHONY: all clean

all: $(OUTPUT_PATH)/common/hero_runtime_tools.o
	@$(LOG_SUCCESS)

clean:
	$(Q)$(CLEAN_RM_CMD) $(OUTPUT_PATH)/common/hero_runtime_tools.o
