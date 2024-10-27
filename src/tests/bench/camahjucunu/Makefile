# -----------------------------------------------------------------
# Dependencies - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config
# -----------------------------------------------------------------
# Local Makefile Variables - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
HERE_PATH := $(ROOT_PATH)/tests/bench/camahjucunu
TEST_BUILD := $(ROOT_PATH)/tests/build
# USUAL_LINK_CMD = $(LOG_START) && $(GXX) $(GXX_FLAGS) $(USUAL_INCLUDE_PATHS) $(OBJECT_TO_LINK) $(USUAL_LD_FLAGS) $< -o $@ && $(LOG_END)
EXTENDED_LINK_CMD = $(LOG_START) && $(LOCAL_CLEAN) && $(GXX) $(GXX_FLAGS) $(USUAL_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS) $(OBJECT_TO_LINK) $(LIBCURl_LD_LAGS) $(SSL_LD_FLAGS) $(USUAL_LD_FLAGS) $< -o $@ && $(LOG_END)
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_exchange_types_serialization test_exchange_types_serialization
$(TEST_BUILD)/test_exchange_types_serialization: $(HERE_PATH)/test_exchange_types_serialization.cpp
	@$(EXTENDED_LINK_CMD) 
	$(RUN_MODE) $(TEST_BUILD)/test_exchange_types_serialization
test_exchange_types_serialization: $(TEST_BUILD)/test_exchange_types_serialization
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_exchange_types_deserialization test_exchange_types_deserialization
$(TEST_BUILD)/test_exchange_types_deserialization: $(HERE_PATH)/test_exchange_types_deserialization.cpp
	@$(EXTENDED_LINK_CMD)
	$(RUN_MODE) $(TEST_BUILD)/test_exchange_types_deserialization
test_exchange_types_deserialization: $(TEST_BUILD)/test_exchange_types_deserialization
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_binance_testnet test_binance_testnet
$(TEST_BUILD)/test_binance_testnet: $(HERE_PATH)/test_binance_testnet.cpp
	@$(EXTENDED_LINK_CMD) 
	$(RUN_MODE) $(TEST_BUILD)/test_binance_testnet
test_binance_testnet: $(TEST_BUILD)/test_binance_testnet
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_websocket test_websocket
$(TEST_BUILD)/test_websocket: $(HERE_PATH)/test_websocket.cpp
	@$(EXTENDED_LINK_CMD) 
	$(RUN_MODE) $(TEST_BUILD)/test_websocket
test_websocket: $(TEST_BUILD)/test_websocket
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_binance_mech_server test_binance_mech_server
$(TEST_BUILD)/test_binance_mech_server: $(HERE_PATH)/test_binance_mech_server.cpp
	@$(EXTENDED_LINK_CMD)
	$(RUN_MODE) $(TEST_BUILD)/test_binance_mech_server
test_binance_mech_server: $(TEST_BUILD)/test_binance_mech_server
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_binance_mech_data test_binance_mech_data
$(TEST_BUILD)/test_binance_mech_data: $(HERE_PATH)/test_binance_mech_data.cpp
	@$(EXTENDED_LINK_CMD)
	$(RUN_MODE) $(TEST_BUILD)/test_binance_mech_data
test_binance_mech_data: $(TEST_BUILD)/test_binance_mech_data
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_binance_mech_account test_binance_mech_account
$(TEST_BUILD)/test_binance_mech_account: $(HERE_PATH)/test_binance_mech_account.cpp
	@$(EXTENDED_LINK_CMD)
	$(RUN_MODE) $(TEST_BUILD)/test_binance_mech_account
test_binance_mech_account: $(TEST_BUILD)/test_binance_mech_account
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_binance_mech_trade test_binance_mech_trade
$(TEST_BUILD)/test_binance_mech_trade: $(HERE_PATH)/test_binance_mech_trade.cpp
	@$(EXTENDED_LINK_CMD)
	$(RUN_MODE) $(TEST_BUILD)/test_binance_mech_trade
test_binance_mech_trade: $(TEST_BUILD)/test_binance_mech_trade
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_bnf_grammar_lexer test_bnf_grammar_lexer
$(TEST_BUILD)/test_bnf_grammar_lexer: $(HERE_PATH)/test_bnf_grammar_lexer.cpp
	@$(EXTENDED_LINK_CMD)
	$(RUN_MODE) $(TEST_BUILD)/test_bnf_grammar_lexer
test_bnf_grammar_lexer: $(TEST_BUILD)/test_bnf_grammar_lexer
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_bnf_grammar_parser test_bnf_grammar_parser
$(TEST_BUILD)/test_bnf_grammar_parser: $(HERE_PATH)/test_bnf_grammar_parser.cpp
	@$(EXTENDED_LINK_CMD)
	$(RUN_MODE) $(TEST_BUILD)/test_bnf_grammar_parser
test_bnf_grammar_parser: $(TEST_BUILD)/test_bnf_grammar_parser
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_bnf_instruction_lexer test_bnf_instruction_lexer
$(TEST_BUILD)/test_bnf_instruction_lexer: $(HERE_PATH)/test_bnf_instruction_lexer.cpp
	@$(EXTENDED_LINK_CMD)
	$(RUN_MODE) $(TEST_BUILD)/test_bnf_instruction_lexer
test_bnf_instruction_lexer: $(TEST_BUILD)/test_bnf_instruction_lexer
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_bnf_instruction_parser test_bnf_instruction_parser
$(TEST_BUILD)/test_bnf_instruction_parser: $(HERE_PATH)/test_bnf_instruction_parser.cpp
	@$(EXTENDED_LINK_CMD)
	$(RUN_MODE) $(TEST_BUILD)/test_bnf_instruction_parser
test_bnf_instruction_parser: $(TEST_BUILD)/test_bnf_instruction_parser
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_bnf_ast test_bnf_ast
$(TEST_BUILD)/test_bnf_ast: $(HERE_PATH)/test_bnf_ast.cpp
	@$(EXTENDED_LINK_CMD)
	$(RUN_MODE) $(TEST_BUILD)/test_bnf_ast
test_bnf_ast: $(TEST_BUILD)/test_bnf_ast
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_bnf_observation_pipeline test_bnf_observation_pipeline
$(TEST_BUILD)/test_bnf_observation_pipeline: $(HERE_PATH)/test_bnf_observation_pipeline.cpp
	@$(EXTENDED_LINK_CMD)
	$(RUN_MODE) $(TEST_BUILD)/test_bnf_observation_pipeline
test_bnf_observation_pipeline: $(TEST_BUILD)/test_bnf_observation_pipeline
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_binance_dataloader test_binance_dataloader
$(TEST_BUILD)/test_binance_dataloader: $(HERE_PATH)/test_binance_dataloader.cpp
	@$(EXTENDED_LINK_CMD)
	$(RUN_MODE) $(TEST_BUILD)/test_binance_dataloader
test_binance_dataloader: $(TEST_BUILD)/test_binance_dataloader
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