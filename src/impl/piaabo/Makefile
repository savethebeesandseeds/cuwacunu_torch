# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/piaabo
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))
# -----------------------------------------------------------------
# Build Rules (producers; no lib*.a prerequisites)
#   - Pure utilities live in named Piaabo rooms.
# -----------------------------------------------------------------
$(eval $(call BUILD_OBJ,      common,  core/utils,              utils))
$(eval $(call BUILD_OBJ,      common,  parse/json/json_parsing, json_parsing))
$(eval $(call BUILD_OBJ,      common,  io/files,                files))
$(eval $(call BUILD_OBJ_INC,  openssl, security/encryption,     encryption, $(SSL_INCLUDE_PATHS)))
$(eval $(call BUILD_OBJ_INC,  openssl, security/security,       security,   $(SSL_INCLUDE_PATHS)))

PIAABO_CORE := \
  utils \
  json_parsing \
  files \
  encryption \
  security

# -----------------------------------------------------------------
# Aggregate Target
#   - Build local piaabo leaf producers.
#   - Fan out into parse/network/math/tensor rooms so
#     GNU make can schedule them in parallel.
# -----------------------------------------------------------------
.PHONY: network
network:
	$(MAKE) -C $(HERE_PATH)/network/curl all

.PHONY: parse
parse:
	$(MAKE) -C $(HERE_PATH)/parse/bnf all

.PHONY: math
math:
	$(MAKE) -C $(HERE_PATH)/math all

.PHONY: tensor
tensor:
	$(MAKE) -C $(HERE_PATH)/tensor all

.PHONY: db
db:
	$(MAKE) -C $(HERE_PATH)/db all

.PHONY: all
all: $(PIAABO_CORE) network parse math tensor db
	@$(LOG_SUCCESS)
