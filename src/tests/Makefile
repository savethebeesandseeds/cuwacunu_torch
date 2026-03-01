# =============================================================================
# tests/Makefile — explicit dispatcher (prefix-safe)
# =============================================================================
# 0) Root & Config
ROOT_PATH := ..
include $(ROOT_PATH)/Makefile.config

# 1) Local Paths
HERE_PATH := $(TESTS_PATH)

# 2) Defaults (libs/flags)
# (none)

# 3) Tests (dispatcher helpers)
#   FORWARD_ONE(name, dir) generates:
#     - test_<name>: runs in the subdir, trying in order:
#         run-test_<name>  ||  $(TEST_OUT)/test_<name>  ||  test_<name>
#   Defined in Makefile.config: FORWARD_ONE

# --------------------------- BNF Parser Core ---------------------------------
BNF_PARSER_CORE := bnf_grammar_lexer bnf_grammar_parser bnf_instruction_lexer bnf_instruction_parser
$(foreach t,$(BNF_PARSER_CORE),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/piaabo)))

# ------------------------------ DSL Decoders ---------------------------------
DSL := dsl_observation dsl_jkimyei_specs dsl_tsiemene_circuit dsl_tsiemene_wave dsl_canonical_path
$(foreach t,$(DSL),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/camahjucunu)))

# --------------------------- camahjucunu/types -------------------------------
TYPES := types_serialization types_deserialization
$(foreach t,$(TYPES),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/camahjucunu)))

# --------------------------- camahjucunu/exchange ----------------------------
EXCHANGE := binance_testnet binance_mech_server binance_mech_data binance_mech_account binance_mech_trade
$(foreach t,$(EXCHANGE),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/camahjucunu)))

# ------------------------------- piaabo --------------------------------------
PIAABO := dconfig dconfig_contract_lock dsecurity dencryption djson_parsing dfiles_binary_to_vector
$(foreach t,$(PIAABO),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/piaabo)))

# --------------------------- camahjucunu/data --------------------------------
DATA := memory_mapped_datafile memory_mapped_dataset memory_mapped_concat_dataset memory_mapped_concat_dataloader memory_mapped_edge_cases
$(foreach t,$(DATA),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/camahjucunu)))

# ------------------------------ iinuji ---------------------------------------
IINUJI := iinuji_plots iinuji_logs iinuji_suit iinuji_mouse iinuji_tsi
$(foreach t,$(IINUJI),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/iinuji)))

# ------------------------------ iinuji + torch--------------------------------
IINUJI := iinuji_toroid iinuji_data_viz iinuji_timeseries iinuji_cmd iinuji_cmd_home iinuji_cmd_canonical_paths iinuji_cmd_terminal
$(foreach t,$(IINUJI),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/iinuji)))

# ------------------------------ tsiemene--------------------------------
TSIEMENE := tsi_contract_wave board_contract_init
$(foreach t,$(TSIEMENE),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/tsiemene)))

# --------------------------- camahjucunu/https -------------------------------
HTTPS := websocket
$(foreach t,$(HTTPS),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/camahjucunu)))

# --------------------------- camahjucunu/db -------------------------------
DB := db db_rag
$(foreach t,$(DB),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/camahjucunu)))

# ------------------------------ jkimyei --------------------------------------
JKIMYEI := jk_setup
$(foreach t,$(JKIMYEI),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/jkimyei)))

# ------------------------------ wikimyei/VICReg ------------------------------
VICREG := vicreg_4d_train vicreg_4d_projector_gpu_vs_cpu vicreg_4d_save_and_load
$(foreach t,$(VICREG),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/wikimyei)))

# ------------------------------ wikimyei/ts2vec ------------------------------
TS2VEC := ts2vec_datautils ts2vec_losses ts2vec_dilated_conv ts2vec_encoder ts2vec_state_dict ts2vec_clonable ts2vec ts2vec_ECG200
$(foreach t,$(TS2VEC),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/wikimyei)))

# ------------------------------ wikimyei/MDN ---------------------------------
MDN := mixture_density_network
$(foreach t,$(MDN),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/wikimyei)))

# ------------------------------ wikimyei/ExpectedValue -----------------------
EXPVAL := expected_value
$(foreach t,$(EXPVAL),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/wikimyei)))

# 4) Shortcut groups (delegate only; no test logic changes)
.PHONY: parser dsl types exchange piaabo data iinuji iinuji_torch tsiemene https db jkimyei vicreg ts2vec mdn expval
parser: $(addprefix test_,$(BNF_PARSER_CORE))
dsl: $(addprefix test_,$(DSL))
types: $(addprefix test_,$(TYPES))
exchange: $(addprefix test_,$(EXCHANGE))
piaabo: $(addprefix test_,$(PIAABO))
data: $(addprefix test_,$(DATA))
iinuji: test_iinuji_plots test_iinuji_logs test_iinuji_suit test_iinuji_mouse test_iinuji_tsi
iinuji_torch: test_iinuji_toroid test_iinuji_data_viz test_iinuji_timeseries test_iinuji_cmd test_iinuji_cmd_home test_iinuji_cmd_canonical_paths test_iinuji_cmd_terminal
tsiemene: $(addprefix test_,$(TSIEMENE))
https: $(addprefix test_,$(HTTPS))
db: $(addprefix test_,$(DB))
jkimyei: $(addprefix test_,$(JKIMYEI))
vicreg: $(addprefix test_,$(VICREG))
ts2vec: $(addprefix test_,$(TS2VEC))
mdn: $(addprefix test_,$(MDN))
expval: $(addprefix test_,$(EXPVAL))

# Legacy aliases for old parser target names.
.PHONY: test_dsl_parser_grammar_lexer test_dsl_parser_grammar_parser test_dsl_parser_instruction_lexer test_dsl_parser_instruction_parser
test_dsl_parser_grammar_lexer: test_bnf_grammar_lexer
test_dsl_parser_grammar_parser: test_bnf_grammar_parser
test_dsl_parser_instruction_lexer: test_bnf_instruction_lexer
test_dsl_parser_instruction_parser: test_bnf_instruction_parser

.PHONY: test ta clean c
test ta: all
clean c: clean_tests

# 5) Aggregate: all
.PHONY: all
all:
	+$(MAKE) -C $(HERE_PATH)/bench all
	@$(LOG_SUCCESS)

# 6) Clean
.PHONY: clean_tests
clean_tests:
	@echo "$(COLOR_RED) Cleaning test build $(COLOR_RESET)"
	@rm -f $(OUTPUT_PATH)/*.o $(OUTPUT_PATH)/*.d $(OUTPUT_PATH)/test_*
