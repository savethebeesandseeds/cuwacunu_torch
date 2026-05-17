ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config

.DEFAULT_GOAL := all

.PHONY: all idydb

idydb:
	$(MAKE) -C $(IMPL_PATH)/piaabo/db/idydb all

all: idydb
	@printf "[PIAABO:DB] database helpers migrated\n"
