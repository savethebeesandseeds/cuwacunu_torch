# =============================================================================
# tests/Makefile — explicit dispatcher (prefix-safe)
# =============================================================================
ROOT_PATH := ..
include $(ROOT_PATH)/Makefile.config

HERE_PATH := $(TESTS_PATH)

# ------------------------------ BNF ------------------------------------------
BNF := bnf_grammar_lexer bnf_grammar_parser bnf_instruction_lexer bnf_instruction_parser \
       bnf_observation_pipeline bnf_training_components
$(foreach t,$(BNF),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/camahjucunu/bnf)))

# --------------------------- camahjucnu/exchange -----------------------------
EXCHANGE := exchange_types_serialization exchange_types_deserialization binance_testnet \
            binance_mech_server binance_mech_data binance_mech_account binance_mech_trade
$(foreach t,$(EXCHANGE),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/camahjucunu/exchange)))

# ------------------------------- piaabo --------------------------------------
PIAABO := darchitecture dconfig dsecurity dencryption djson_parsing dfiles_binary_to_vector
$(foreach t,$(PIAABO),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/piaabo)))
$(eval $(call FORWARD_ONE,gpu_vs_cpu,$(HERE_PATH)/bench/piaabo/torch_compat))

# --------------------------- camahjucnu/data ---------------------------------
DATA := memory_mapped_datafile memory_mapped_dataset \
        memory_mapped_concat_dataset memory_mapped_concat_dataloader
$(foreach t,$(DATA),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/camahjucunu/data)))

# -------------------------- iinuji/ncurses ---------------------------
IINUJI := iinuji_plots iinuji_logs iinuji_suit iinuji_mouse
$(foreach t,$(IINUJI),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/iinuji)))

# --------------------------- camahjucnu/https --------------------------------
HTTPS := websocket
$(foreach t,$(HTTPS),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/camahjucunu/https)))

# --------------------------- jkimyei/training_setup ---------------------
JKIMYEI := jk_setup
$(foreach t,$(JKIMYEI),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/jkimyei/training_setup)))

# ---------------- wikimyei / VICReg ------------------------------------------
VICREG := vicreg_4d_train vicreg_4d_projector_gpu_vs_cpu vicreg_4d_save_and_load
$(foreach t,$(VICREG),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/wikimyei/heuristics/representation_learning/VICReg)))

# ---------------- wikimyei / MDN ------------------------------------------
MDN := mixture_density_network
$(foreach t,$(MDN),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/wikimyei/heuristics/semantic_learning/mdn)))

# ---------------- wikimyei / Expected Value ------------------------------------------
MDN := expected_value
$(foreach t,$(MDN),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/wikimyei/heuristics/semantic_learning/expected_value)))

# ------------------------------ meta -----------------------------------------
.PHONY: all
all:
	$(MAKE) -C $(HERE_PATH)/bench all
	@$(LOG_SUCCESS)

.PHONY: clean_tests
clean_tests:
	@echo "$(COLOR_RED) Cleaning test build $(COLOR_RESET)"
	@rm -f $(OUTPUT_PATH)/*.o $(OUTPUT_PATH)/*.d $(OUTPUT_PATH)/test_*
