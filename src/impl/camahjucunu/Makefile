# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/camahjucunu
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))
# -----------------------------------------------------------------
# Aggregate Targets
#   - This directory doesn’t build objects itself.
#   - It fans out into submodules so GNU make can schedule them in parallel.
# -----------------------------------------------------------------
.PHONY: dsl
dsl:
	$(MAKE) -C $(HERE_PATH)/dsl all

.PHONY: db
db:
	$(MAKE) -C $(HERE_PATH)/db all

.PHONY: exchange
exchange:
	$(MAKE) -C $(HERE_PATH)/exchange all

.PHONY: data
data:
	$(MAKE) -C $(HERE_PATH)/data all

.PHONY: all
all: dsl db exchange data
	@$(LOG_SUCCESS)
