ROOT_PATH := ./src
# Load the shared build policy here so repo-root invocations stay the true
# top-level make. Otherwise `make -C /cuwacunu ...` turns `src/` into
# MAKELEVEL=1, which suppresses the build-mode banner and top-level `-j`
# injection handled by src/Makefile.config.
include $(ROOT_PATH)/Makefile.config
.DEFAULT_GOAL := lib

.PHONY: lib link install modules tests clean help targets

lib:
	+$(MAKE) -C $(ROOT_PATH) lib

link:
	+$(MAKE) -C $(ROOT_PATH) link

install:
	+$(MAKE) -C $(ROOT_PATH) install

modules:
	+$(MAKE) -C $(ROOT_PATH) modules

tests:
	+$(MAKE) -C $(ROOT_PATH) tests

clean:
	+$(MAKE) -C $(ROOT_PATH) clean

help:
	@echo "repo targets:"
	@echo "  lib   canonical library/core aggregate"
	@echo "  link  canonical final-linked executable aggregate"
	@echo "  install  finalize in-place hero/tool outputs after link"
	@echo "related:"
	@echo "  tests modules clean"
	@echo "advanced/internal:"
	@echo "  use 'make -C src/main all' only when you intentionally want the direct main tree"

targets:
	@printf "%s\n" lib link install modules tests clean help targets
