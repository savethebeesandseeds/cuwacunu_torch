# =============================================================================
# tests/bench/camahjucunu/dsl_parser/Makefile — DSL parser core tests
# =============================================================================
# 0) Root & Config
ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config

# 1) Local Paths
HERE_PATH  := $(TESTS_PATH)/bench/camahjucunu/dsl_parser
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

# 2) Defaults (libs/flags)
# (use defaults from Makefile.config)

# 3) Tests (build + run aliases)
$(eval $(call TEST_ONEFILE, test_dsl_parser_grammar_lexer,        test_dsl_parser_grammar_lexer.cpp))
$(eval $(call TEST_ONEFILE, test_dsl_parser_grammar_parser,       test_dsl_parser_grammar_parser.cpp))
$(eval $(call TEST_ONEFILE, test_dsl_parser_instruction_lexer,    test_dsl_parser_instruction_lexer.cpp))
$(eval $(call TEST_ONEFILE, test_dsl_parser_instruction_parser,   test_dsl_parser_instruction_parser.cpp))

# 4) Target-specific includes/link flags
# (none)

# 5) Aggregate: all
.PHONY: all
all: \
	$(TEST_OUT)/test_dsl_parser_grammar_lexer \
	$(TEST_OUT)/test_dsl_parser_grammar_parser \
	$(TEST_OUT)/test_dsl_parser_instruction_lexer \
	$(TEST_OUT)/test_dsl_parser_instruction_parser
	@$(LOG_SUCCESS)

# 6) Aggregate: run
.PHONY: run
run: \
	run-test_dsl_parser_grammar_lexer \
	run-test_dsl_parser_grammar_parser \
	run-test_dsl_parser_instruction_lexer \
	run-test_dsl_parser_instruction_parser

# 7) Clean
.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/test_dsl_parser_*
