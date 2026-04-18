# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/camahjucunu/exchange
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))

# -----------------------------------------------------------------
# Aggregate Targets
# -----------------------------------------------------------------
.PHONY: binance
binance:
	$(MAKE) -C $(HERE_PATH)/binance all

.PHONY: all
all: binance
	@$(LOG_SUCCESS)
