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

# ------------------------------ BNF ------------------------------------------
BNF := bnf_grammar_lexer bnf_grammar_parser bnf_instruction_lexer bnf_instruction_parser \
       bnf_observation_pipeline bnf_training_components
$(foreach t,$(BNF),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/camahjucunu/bnf)))

# --------------------------- camahjucunu/types -------------------------------
TYPES := types_serialization types_deserialization
$(foreach t,$(TYPES),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/camahjucunu/types)))

# --------------------------- camahjucunu/exchange ----------------------------
EXCHANGE := binance_testnet binance_mech_server binance_mech_data binance_mech_account binance_mech_trade
$(foreach t,$(EXCHANGE),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/camahjucunu/exchange)))

# ------------------------------- piaabo --------------------------------------
PIAABO := darchitecture dconfig dsecurity dencryption djson_parsing dfiles_binary_to_vector
$(foreach t,$(PIAABO),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/piaabo)))

# --------------------------- camahjucunu/data --------------------------------
DATA := memory_mapped_datafile memory_mapped_dataset memory_mapped_concat_dataset memory_mapped_concat_dataloader
$(foreach t,$(DATA),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/camahjucunu/data)))

# ------------------------------ iinuji ---------------------------------------
IINUJI := iinuji_plots iinuji_logs iinuji_suit iinuji_mouse
$(foreach t,$(IINUJI),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/iinuji)))

# ------------------------------ iinuji + torch--------------------------------
IINUJI := iinuji_toroid iinuji_data_viz
$(foreach t,$(IINUJI),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/iinuji/torch_include)))

# --------------------------- camahjucunu/https -------------------------------
HTTPS := websocket
$(foreach t,$(HTTPS),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/camahjucunu/https)))

# --------------------------- jkimyei/training_setup --------------------------
JKIMYEI := jk_setup
$(foreach t,$(JKIMYEI),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/jkimyei/training_setup)))

# ------------------------------ wikimyei/VICReg ------------------------------
VICREG := vicreg_4d_train vicreg_4d_projector_gpu_vs_cpu vicreg_4d_save_and_load
$(foreach t,$(VICREG),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/wikimyei/heuristics/representation_learning/VICReg)))

# ------------------------------ wikimyei/ts2vec ------------------------------
TS2VEC := ts2vec_datautils ts2vec_losses ts2vec_dilated_conv ts2vec_encoder ts2vec_state_dict ts2vec_clonable ts2vec ts2vec_ECG200
$(foreach t,$(TS2VEC),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/wikimyei/heuristics/representation_learning/ts2vec)))

# ------------------------------ wikimyei/MDN ---------------------------------
MDN := mixture_density_network
$(foreach t,$(MDN),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/wikimyei/heuristics/semantic_learning/mdn)))

# ------------------------------ wikimyei/ExpectedValue -----------------------
EXPVAL := expected_value
$(foreach t,$(EXPVAL),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/wikimyei/heuristics/semantic_learning/expected_value)))

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
