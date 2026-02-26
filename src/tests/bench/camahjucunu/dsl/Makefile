# =============================================================================
# tests/bench/camahjucunu/dsl/Makefile — DSL decoder tests
# =============================================================================
# 0) Root & Config
ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config

# 1) Local Paths
HERE_PATH  := $(TESTS_PATH)/bench/camahjucunu/dsl
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

# 2) Defaults (libs/flags)
# (use defaults from Makefile.config)

# 3) Tests (build + run aliases)
$(eval $(call TEST_ONEFILE, test_dsl_observation_pipeline, test_dsl_observation_pipeline.cpp))
$(eval $(call TEST_ONEFILE, test_dsl_jkimyei_specs,        test_dsl_jkimyei_specs.cpp))
$(eval $(call TEST_ONEFILE, test_dsl_tsiemene_circuit,     test_dsl_tsiemene_circuit.cpp))
$(eval $(call TEST_ONEFILE, test_dsl_canonical_path,       test_dsl_canonical_path.cpp))

# 4) Target-specific includes/link flags
# (none)

# 5) Aggregate: all
.PHONY: all
all: \
	$(TEST_OUT)/test_dsl_observation_pipeline \
	$(TEST_OUT)/test_dsl_jkimyei_specs \
	$(TEST_OUT)/test_dsl_tsiemene_circuit \
	$(TEST_OUT)/test_dsl_canonical_path
	@$(LOG_SUCCESS)

# 6) Aggregate: run
.PHONY: run
run: \
	run-test_dsl_observation_pipeline \
	run-test_dsl_jkimyei_specs \
	run-test_dsl_tsiemene_circuit \
	run-test_dsl_canonical_path

# 7) Clean
.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/test_dsl_*
