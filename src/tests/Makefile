# -----------------------------------------------------------------
# Dependencies - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
ROOT_PATH := ..
include $(ROOT_PATH)/Makefile.config
# -----------------------------------------------------------------
# Local Makefile Variables - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
HERE_PATH := $(ROOT_PATH)/tests
TEST_BUILD := $(HERE_PATH)/build
# -----------------------------------------------------------------
# - - - Especific Targets - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------

# -----------------------------------------------------------------
# - - - Piaabo -- - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
test_darchitecture: 
	$(MAKE) -C $(HERE_PATH)/bench/piaabo test_darchitecture
test_dconfig: 
	$(MAKE) -C $(HERE_PATH)/bench/piaabo test_dconfig
test_dsecurity: 
	$(MAKE) -C $(HERE_PATH)/bench/piaabo test_dsecurity
test_dencryption: 
	$(MAKE) -C $(HERE_PATH)/bench/piaabo test_dencryption
test_json_parsing:
	$(MAKE) -C $(HERE_PATH)/bench/piaabo test_json_parsing
# -----------------------------------------------------------------
# - - - Camahjucnu -- - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
test_websocket: 
	$(MAKE) -C $(HERE_PATH)/bench/camahjucunu/https test_websocket
test_exchange_types_serialization: 
	$(MAKE) -C $(HERE_PATH)/bench/camahjucunu/exchange test_exchange_types_serialization
test_exchange_types_deserialization: 
	$(MAKE) -C $(HERE_PATH)/bench/camahjucunu/exchange test_exchange_types_deserialization
test_binance_testnet: 
	$(MAKE) -C $(HERE_PATH)/bench/camahjucunu/exchange test_binance_testnet
test_binance_mech_server: 
	$(MAKE) -C $(HERE_PATH)/bench/camahjucunu/exchange test_binance_mech_server
test_binance_mech_data: 
	$(MAKE) -C $(HERE_PATH)/bench/camahjucunu/exchange test_binance_mech_data
test_binance_mech_account: 
	$(MAKE) -C $(HERE_PATH)/bench/camahjucunu/exchange test_binance_mech_account
test_binance_mech_trade: 
	$(MAKE) -C $(HERE_PATH)/bench/camahjucunu/exchange test_binance_mech_trade
test_bnf_grammar_lexer: 
	$(MAKE) -C $(HERE_PATH)/bench/camahjucunu/bnf test_bnf_grammar_lexer
test_bnf_grammar_parser: 
	$(MAKE) -C $(HERE_PATH)/bench/camahjucunu/bnf test_bnf_grammar_parser
test_bnf_instruction_lexer: 
	$(MAKE) -C $(HERE_PATH)/bench/camahjucunu/bnf test_bnf_instruction_lexer
test_bnf_instruction_parser: 
	$(MAKE) -C $(HERE_PATH)/bench/camahjucunu/bnf test_bnf_instruction_parser
test_bnf_observation_pipeline: 
	$(MAKE) -C $(HERE_PATH)/bench/camahjucunu/bnf test_bnf_observation_pipeline
test_dataloader:
	$(MAKE) -C $(HERE_PATH)/bench/camahjucunu/data test_dataloader
# -----------------------------------------------------------------
# - - - Wikimyei -- - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
test_entropy_methods:
	$(MAKE) -C $(HERE_PATH)/bench/wikimyei test_entropy_methods
# -----------------------------------------------------------------
# - - - General Target -- - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: all
all: 
	$(MAKE) -C $(HERE_PATH)/bench all
	@$(LOG_SUCCESS)
.PHONY: clean_tests
clean_tests:
	@echo "$(COLOR_RED) Cleaning test build $(COLOR_RESET)"
	shred -u $(TEST_BUILD)/*.d $(TEST_BUILD)/*.o
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------