ROOT_PATH := .
include $(ROOT_PATH)/Makefile.config

.DEFAULT_GOAL := lib

.PHONY: lib modules piaabo wikimyei ujcamei jkimyei kikijyeba hero link install tests clean help targets

lib modules:
	@printf "[SRC] migrated implementation Makefiles exist; default build waits for dependencies\n"

piaabo:
	@printf "[SRC] piaabo utility sources migrated; build waits for remaining legacy dependencies\n"

wikimyei:
	@printf "[SRC] wikimyei implementation Makefiles exist; build waits for dependencies\n"

ujcamei:
	$(MAKE) -C $(IMPL_PATH)/ujcamei all

jkimyei:
	$(MAKE) -C $(IMPL_PATH)/jkimyei all

kikijyeba:
	$(MAKE) -C $(IMPL_PATH)/kikijyeba all

hero:
	$(MAKE) -C $(IMPL_PATH)/hero all

link install:
	@printf "[SRC] fresh source root: no binaries defined yet\n"

tests:
	@printf "[SRC] fresh source root: no tests defined yet\n"

clean:
	$(Q)$(CLEAN_TREE_CMD) "$(OUTPUT_PATH)" "$(LIB_OUT)" "$(TEST_OUT)"

help:
	@printf "%s\n" \
	  "fresh src targets:" \
	  "  lib      marker until migrated dependencies exist" \
	  "  modules  alias for lib" \
	  "  piaabo   marker for migrated Piaabo utility sources" \
	  "  wikimyei marker for migrated Wikimyei implementation Makefiles" \
	  "  ujcamei build Ujcamei implementation modules" \
	  "  jkimyei build Jkimyei implementation modules" \
	  "  kikijyeba build Kikijyeba implementation modules" \
	  "  hero    build Hero implementation modules" \
	  "  tests    no-op until fresh tests exist" \
	  "  link     no-op until fresh binaries exist" \
	  "  install  no-op until fresh binaries exist" \
	  "  clean    no-op until fresh build artifacts exist"

targets:
	@printf "%s\n" lib modules piaabo wikimyei ujcamei jkimyei kikijyeba hero tests link install clean help targets
