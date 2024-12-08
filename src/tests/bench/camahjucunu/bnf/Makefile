# -----------------------------------------------------------------
# Dependencies - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config
# -----------------------------------------------------------------
# Local Makefile Variables - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
HERE_PATH := $(ROOT_PATH)/tests/bench/camahjucunu/bnf
TEST_BUILD := $(ROOT_PATH)/tests/build
BNF_LINK_CMD = $(LOG_START) && $(LOCAL_CLEAN) && $(GXX) $(GXX_FLAGS) $(USUAL_INCLUDE_PATHS) $(OUTPUT_PATH)/common/*.o $(USUAL_LD_FLAGS) $< -o $@ && $(LOG_END)
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_bnf_grammar_lexer test_bnf_grammar_lexer
$(TEST_BUILD)/test_bnf_grammar_lexer: $(HERE_PATH)/test_bnf_grammar_lexer.cpp
	@$(BNF_LINK_CMD)
	$(RUN_MODE) $(TEST_BUILD)/test_bnf_grammar_lexer
test_bnf_grammar_lexer: $(TEST_BUILD)/test_bnf_grammar_lexer
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_bnf_grammar_parser test_bnf_grammar_parser
$(TEST_BUILD)/test_bnf_grammar_parser: $(HERE_PATH)/test_bnf_grammar_parser.cpp
	@$(BNF_LINK_CMD)
	$(RUN_MODE) $(TEST_BUILD)/test_bnf_grammar_parser
test_bnf_grammar_parser: $(TEST_BUILD)/test_bnf_grammar_parser
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_bnf_instruction_lexer test_bnf_instruction_lexer
$(TEST_BUILD)/test_bnf_instruction_lexer: $(HERE_PATH)/test_bnf_instruction_lexer.cpp
	@$(BNF_LINK_CMD)
	$(RUN_MODE) $(TEST_BUILD)/test_bnf_instruction_lexer
test_bnf_instruction_lexer: $(TEST_BUILD)/test_bnf_instruction_lexer
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_bnf_instruction_parser test_bnf_instruction_parser
$(TEST_BUILD)/test_bnf_instruction_parser: $(HERE_PATH)/test_bnf_instruction_parser.cpp
	@$(BNF_LINK_CMD)
	$(RUN_MODE) $(TEST_BUILD)/test_bnf_instruction_parser
test_bnf_instruction_parser: $(TEST_BUILD)/test_bnf_instruction_parser
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_bnf_observation_pipeline test_bnf_observation_pipeline
$(TEST_BUILD)/test_bnf_observation_pipeline: $(HERE_PATH)/test_bnf_observation_pipeline.cpp
	@$(BNF_LINK_CMD)
	$(RUN_MODE) $(TEST_BUILD)/test_bnf_observation_pipeline
test_bnf_observation_pipeline: $(TEST_BUILD)/test_bnf_observation_pipeline
# -----------------------------------------------------------------
# - - - General Target -- - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: all
all: \
	$(TEST_BUILD)/test_exchange_types_serialization \
	$(TEST_BUILD)/test_exchange_types_deserialization \
	$(TEST_BUILD)/test_websocket \
	$(TEST_BUILD)/test_binance_mech_server \
	$(TEST_BUILD)/test_binance_mech_data \
	$(TEST_BUILD)/test_binance_mech_account \
	$(TEST_BUILD)/test_binance_mech_trade \
	$(TEST_BUILD)/test_binance_dataloader 
	@$(LOG_SUCCESS)
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------