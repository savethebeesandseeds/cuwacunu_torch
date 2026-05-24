ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config

HERE_PATH  := $(TESTS_PATH)/bench/kikijyeba/lattice_exposure
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

$(eval $(call TEST_ONEFILE, test_kikijyeba_lattice_exposure, test_kikijyeba_lattice_exposure.cpp))
$(eval $(call TEST_ONEFILE, test_kikijyeba_lattice_performance_budget, test_kikijyeba_lattice_performance_budget.cpp))

.PHONY: all
all: $(TEST_OUT)/test_kikijyeba_lattice_exposure $(TEST_OUT)/test_kikijyeba_lattice_performance_budget
	@$(LOG_SUCCESS)

.PHONY: run
run: run-test_kikijyeba_lattice_exposure

.PHONY: run-performance-budget
run-performance-budget: run-test_kikijyeba_lattice_performance_budget

.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/test_kikijyeba_lattice_exposure $(TEST_OUT)/test_kikijyeba_lattice_performance_budget
