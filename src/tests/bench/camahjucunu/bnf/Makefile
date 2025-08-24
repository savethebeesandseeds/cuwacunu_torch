# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------
ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config

# -----------------------------------------------------------------------------
# Local Paths
# -----------------------------------------------------------------------------
HERE_PATH  := $(TESTS_PATH)/bench/camahjucunu/bnf
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

# -----------------------------------------------------------------------------
# Build + run aliases
# -----------------------------------------------------------------------------
$(eval $(call TEST_ONEFILE,test_bnf_grammar_lexer,        test_bnf_grammar_lexer.cpp))
$(eval $(call TEST_ONEFILE,test_bnf_grammar_parser,       test_bnf_grammar_parser.cpp))
$(eval $(call TEST_ONEFILE,test_bnf_instruction_lexer,    test_bnf_instruction_lexer.cpp))
$(eval $(call TEST_ONEFILE,test_bnf_instruction_parser,   test_bnf_instruction_parser.cpp))
$(eval $(call TEST_ONEFILE,test_bnf_observation_pipeline, test_bnf_observation_pipeline.cpp))
$(eval $(call TEST_ONEFILE,test_bnf_training_components,  test_bnf_training_components.cpp))

# -----------------------------------------------------------------------------
# Aggregate build
# -----------------------------------------------------------------------------
.PHONY: all
all: \
	$(OUTPUT_PATH)/test_bnf_grammar_lexer \
	$(OUTPUT_PATH)/test_bnf_grammar_parser \
	$(OUTPUT_PATH)/test_bnf_instruction_lexer \
	$(OUTPUT_PATH)/test_bnf_instruction_parser \
	$(OUTPUT_PATH)/test_bnf_observation_pipeline \
	$(OUTPUT_PATH)/test_bnf_training_components
	@$(LOG_SUCCESS)

# -----------------------------------------------------------------------------
# Aggregate run
# -----------------------------------------------------------------------------
.PHONY: run
run: \
     run-test_bnf_grammar_lexer \
     run-test_bnf_grammar_parser \
     run-test_bnf_instruction_lexer \
     run-test_bnf_instruction_parser \
     run-test_bnf_observation_pipeline \
     run-test_bnf_training_components

# -----------------------------------------------------------------------------
# Clean
# -----------------------------------------------------------------------------
.PHONY: clean
clean:
	@rm -f $(OUTPUT_PATH)/test_bnf_*
