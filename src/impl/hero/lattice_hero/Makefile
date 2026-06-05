ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config

.DEFAULT_GOAL := all

$(OUTPUT_PATH)/common/hero_lattice_tools.o: \
	$(IMPL_PATH)/hero/lattice_hero/hero_lattice_tools.cpp \
	$(SRC_ROOT)/Makefile.config
	$(CC_RULE)

.PHONY: all clean lattice

lattice:
	$(MAKE) -C $(IMPL_PATH)/hero/lattice_hero/lattice all

all: $(OUTPUT_PATH)/common/hero_lattice_tools.o lattice
	@$(LOG_SUCCESS)

clean:
	$(Q)$(CLEAN_RM_CMD) $(OUTPUT_PATH)/common/hero_lattice_tools.o
