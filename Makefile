ROOT_PATH := ./src
# Load the shared build policy here so repo-root invocations stay the true
# top-level make. Otherwise `make -C /cuwacunu ...` turns `src/` into
# MAKELEVEL=1, which suppresses the build-mode banner and top-level `-j`
# injection handled by src/Makefile.config.
include $(ROOT_PATH)/Makefile.config
.DEFAULT_GOAL := lib

.PHONY: lib link install modules tests doc doc_fast clean clean-doc help targets

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

doc:
	@printf "[DOC] entering doc build surface\n"
	+$(MAKE) -C doc publish

doc_fast:
	@printf "[DOC] entering fast doc build surface\n"
	+$(MAKE) -C doc fast

clean:
	+$(MAKE) -C $(ROOT_PATH) clean

clean-doc:
	+$(MAKE) -C doc clean

help:
	@echo "repo targets:"
	@echo "  lib   canonical library/core aggregate"
	@echo "  link  canonical final-linked executable aggregate"
	@echo "  install  finalize in-place hero/tool outputs after link"
	@echo "  doc   build the formal algorithm PDF"
	@echo "  doc_fast  build a one-pass draft algorithm PDF"
	@echo "related:"
	@echo "  tests modules clean clean-doc"
	@echo "advanced/internal:"
	@echo "  use 'make -C src/main all' only when you intentionally want the direct main tree"

targets:
	@printf "%s\n" lib link install modules tests doc doc_fast clean clean-doc help targets
