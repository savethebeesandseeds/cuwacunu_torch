# =============================================================================
# cuwacunu/Makefile — runtime executable aggregator
# =============================================================================
ROOT_PATH := ..
include $(ROOT_PATH)/Makefile.config

HERE_PATH := $(ROOT_PATH)/cuwacunu

.PHONY: hero
hero:
	+$(MAKE) -C $(HERE_PATH)/hero all

.PHONY: tools
tools:
	+$(MAKE) -C $(HERE_PATH)/tools all

.PHONY: all
all: hero tools
	@$(LOG_SUCCESS)

.PHONY: clean
clean:
	+$(MAKE) -C $(HERE_PATH)/hero clean
	+$(MAKE) -C $(HERE_PATH)/tools clean
