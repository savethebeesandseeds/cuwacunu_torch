# =============================================================================
# tests/bench/camahjucunu/bnf/Makefile — BNF tests (lexer/parser/pipelines)
# =============================================================================
# 0) Root & Config
ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config

# 1) Local Paths
HERE_PATH  := $(TESTS_PATH)/bench/camahjucunu/bnf
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

# 2) Defaults (libs/flags)
TEST_DEFAULT_LDLIBS :=

# 3) Tests (build + run aliases)
$(eval $(call TEST_ONEFILE, test_bnf_grammar_lexer,        test_bnf_grammar_lexer.cpp))
$(eval $(call TEST_ONEFILE, test_bnf_grammar_parser,       test_bnf_grammar_parser.cpp))
$(eval $(call TEST_ONEFILE, test_bnf_instruction_lexer,    test_bnf_instruction_lexer.cpp))
$(eval $(call TEST_ONEFILE, test_bnf_instruction_parser,   test_bnf_instruction_parser.cpp))
$(eval $(call TEST_ONEFILE, test_bnf_observation_pipeline, test_bnf_observation_pipeline.cpp))
$(eval $(call TEST_ONEFILE, test_bnf_training_components,  test_bnf_training_components.cpp))

# 4) Target-specific includes/link flags
# (none)

# 5) Aggregate: all
.PHONY: all
all: \
	$(TEST_OUT)/test_bnf_grammar_lexer \
	$(TEST_OUT)/test_bnf_grammar_parser \
	$(TEST_OUT)/test_bnf_instruction_lexer \
	$(TEST_OUT)/test_bnf_instruction_parser \
	$(TEST_OUT)/test_bnf_observation_pipeline \
	$(TEST_OUT)/test_bnf_training_components
	@$(LOG_SUCCESS)

# 6) Aggregate: run
.PHONY: run
run: \
	run-test_bnf_grammar_lexer \
	run-test_bnf_grammar_parser \
	run-test_bnf_instruction_lexer \
	run-test_bnf_instruction_parser \
	run-test_bnf_observation_pipeline \
	run-test_bnf_training_components

# 7) Clean
.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/test_bnf_*
