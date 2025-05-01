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
test_djson_parsing:
	$(MAKE) -C $(HERE_PATH)/bench/piaabo test_djson_parsing
test_dfiles_binary_to_vector:
	$(MAKE) -C $(HERE_PATH)/bench/piaabo test_dfiles_binary_to_vector
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
# -----------------------------------------------------------------
# - - - BNF - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
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
test_bnf_training_pipeline:
	$(MAKE) -C $(HERE_PATH)/bench/camahjucunu/bnf test_bnf_training_pipeline
# -----------------------------------------------------------------
# - - - Data - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
test_memory_mapped_concat_dataloader:
	$(MAKE) -C $(HERE_PATH)/bench/camahjucunu/data test_memory_mapped_concat_dataloader
test_memory_mapped_datafile:
	$(MAKE) -C $(HERE_PATH)/bench/camahjucunu/data test_memory_mapped_datafile
test_memory_mapped_dataset:
	$(MAKE) -C $(HERE_PATH)/bench/camahjucunu/data test_memory_mapped_dataset
test_memory_mapped_concat_dataset:
	$(MAKE) -C $(HERE_PATH)/bench/camahjucunu/data test_memory_mapped_concat_dataset
# -----------------------------------------------------------------
# - - - Iinuji - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
test_iinuji_plots:
	$(MAKE) -C $(HERE_PATH)/bench/camahjucunu/GUI/ncurses test_iinuji_plots
test_iinuji_logs:
	$(MAKE) -C $(HERE_PATH)/bench/camahjucunu/GUI/ncurses test_iinuji_logs
test_iinuji_render:
	$(MAKE) -C $(HERE_PATH)/bench/camahjucunu/GUI/ncurses test_iinuji_render
# -----------------------------------------------------------------
# - - - Wikimyei -- - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
test_entropy_methods:
	$(MAKE) -C $(HERE_PATH)/bench/wikimyei test_entropy_methods
test_soft_dtw:
	$(MAKE) -C $(HERE_PATH)/bench/wikimyei/heuristics/ts-tcc test_soft_dtw
test_time_contrastive_coding:
	$(MAKE) -C $(HERE_PATH)/bench/wikimyei/heuristics/ts-tcc test_time_contrastive_coding
test_ts2vec_datautils:
	$(MAKE) -C $(HERE_PATH)/bench/wikimyei/heuristics/ts2vec test_ts2vec_datautils
test_ts2vec_losses:
	$(MAKE) -C $(HERE_PATH)/bench/wikimyei/heuristics/ts2vec test_ts2vec_losses
test_ts2vec_dilated_conv:
	$(MAKE) -C $(HERE_PATH)/bench/wikimyei/heuristics/ts2vec test_ts2vec_dilated_conv
test_ts2vec_encoder:
	$(MAKE) -C $(HERE_PATH)/bench/wikimyei/heuristics/ts2vec test_ts2vec_encoder
test_ts2vec_clonable:
	$(MAKE) -C $(HERE_PATH)/bench/wikimyei/heuristics/ts2vec test_ts2vec_clonable
test_ts2vec_state_dict:
	$(MAKE) -C $(HERE_PATH)/bench/wikimyei/heuristics/ts2vec test_ts2vec_state_dict
test_ts2vec:
	$(MAKE) -C $(HERE_PATH)/bench/wikimyei/heuristics/ts2vec test_ts2vec
test_ts2vec_ECG200:
	$(MAKE) -C $(HERE_PATH)/bench/wikimyei/heuristics/ts2vec test_ts2vec_ECG200
test_vicreg_4d_observation_pipeline:
	$(MAKE) -C $(HERE_PATH)/bench/wikimyei/heuristics/VICReg test_vicreg_4d_observation_pipeline
# -----------------------------------------------------------------
# - - - General Target -- - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY:all
all:
	$(MAKE) -C $(HERE_PATH)/bench all
	@$(LOG_SUCCESS)
.PHONY:clean_tests
clean_tests:
	@echo "$(COLOR_RED) Cleaning test build $(COLOR_RESET)"
	shred -u $(TEST_BUILD)/*.d $(TEST_BUILD)/*.o
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------