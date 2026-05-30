# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../../../../
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/camahjucunu/dsl/jkimyei_specs
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))
# -----------------------------------------------------------------
# Build Rules
# -----------------------------------------------------------------
$(eval $(call BUILD_OBJ, common, jkimyei_specs, dsl_jkimyei_specs))

JKIMYEI_SPECS_PARTS := \
	$(HERE_PATH)/jkimyei_specs_private_parse.cpp \
	$(HERE_PATH)/jkimyei_specs_private_schema.cpp \
	$(HERE_PATH)/jkimyei_specs_private_materialize.cpp \
	$(HERE_PATH)/jkimyei_specs_public.cpp

$(OUTPUT_PATH)/common/jkimyei_specs.o: $(JKIMYEI_SPECS_PARTS)

# -----------------------------------------------------------------
# Aggregate Target
# -----------------------------------------------------------------
.PHONY: all
all: dsl_jkimyei_specs
	@$(LOG_SUCCESS)
