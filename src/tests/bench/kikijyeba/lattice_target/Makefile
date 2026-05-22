ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config

HERE_PATH  := $(TESTS_PATH)/bench/kikijyeba/lattice_target
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

$(eval $(call TEST_ONEFILE, test_kikijyeba_lattice_target, test_kikijyeba_lattice_target.cpp))

.PHONY: all
all: $(TEST_OUT)/test_kikijyeba_lattice_target
	@$(LOG_SUCCESS)

.PHONY: run
run: run-test_kikijyeba_lattice_target

.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/test_kikijyeba_lattice_target
