ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config

HERE_PATH  := $(TESTS_PATH)/bench/kikijyeba/lattice_exposure
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

$(eval $(call TEST_ONEFILE, test_kikijyeba_lattice_exposure, test_kikijyeba_lattice_exposure.cpp))

.PHONY: all
all: $(TEST_OUT)/test_kikijyeba_lattice_exposure
	@$(LOG_SUCCESS)

.PHONY: run
run: run-test_kikijyeba_lattice_exposure

.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/test_kikijyeba_lattice_exposure
