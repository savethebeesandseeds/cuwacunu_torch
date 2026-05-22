ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/hero/config_hero
REL_MODULE := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))

.DEFAULT_GOAL := all

$(eval $(call BUILD_OBJ_INC, common, hero_config_store, hero_config_store, $(SSL_INCLUDE_PATHS)))
$(eval $(call BUILD_OBJ_INC, common, hero_config_tools, hero_config_tools, $(SSL_INCLUDE_PATHS)))

.PHONY: all

all: hero_config_store hero_config_tools
	@$(LOG_SUCCESS)
