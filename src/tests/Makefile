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
DSL := dsl_observation dsl_latent_lineage_state dsl_jkimyei_specs dsl_tsiemene_circuit dsl_iitepi_wave dsl_canonical_path
$(foreach t,$(DSL),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/camahjucunu)))

# --------------------------- camahjucunu/types -------------------------------
TYPES := types_serialization types_deserialization
$(foreach t,$(TYPES),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/camahjucunu)))

# --------------------------- camahjucunu/exchange ----------------------------
EXCHANGE := binance_testnet binance_mech_server binance_mech_data binance_mech_account binance_mech_trade
$(foreach t,$(EXCHANGE),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/camahjucunu)))

# ------------------------------- piaabo --------------------------------------
PIAABO := dconfig dconfig_contract_lock dsecurity dencryption djson_parsing dfiles_binary_to_vector runtime_lls
$(foreach t,$(PIAABO),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/piaabo)))

# --------------------------- camahjucunu/data --------------------------------
DATA := memory_mapped_datafile memory_mapped_dataset memory_mapped_concat_dataset memory_mapped_concat_dataloader memory_mapped_edge_cases
$(foreach t,$(DATA),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/camahjucunu)))

# ------------------------------ iinuji ---------------------------------------
IINUJI := iinuji_plots iinuji_logs iinuji_suit iinuji_mouse iinuji_tsi, iinuji_bootstrap
$(foreach t,$(IINUJI),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/iinuji)))

# ------------------------------ iinuji + torch--------------------------------
IINUJI := iinuji_toroid iinuji_data_viz iinuji_timeseries iinuji_cmd iinuji_cmd_home iinuji_cmd_canonical_paths iinuji_cmd_terminal
$(foreach t,$(IINUJI),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/iinuji)))

# ------------------------------ tsiemene--------------------------------
TSIEMENE := tsi_wikimyei_jkimyei_facet
$(foreach t,$(TSIEMENE),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/tsiemene)))

# ------------------------------ iitepi----------------------------------
IITEPI := network_analytics network_analytics_metrics entropic_capacity_analytics
$(foreach t,$(IITEPI),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/iitepi)))

# --------------------------- camahjucunu/https -------------------------------
HTTPS := websocket
$(foreach t,$(HTTPS),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/camahjucunu)))

# --------------------------- camahjucunu/db -------------------------------
DB := db db_rag db_query_utils
$(foreach t,$(DB),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/camahjucunu)))

# --------------------------- hashimyei -------------------------------
HASHIMYEI := hashimyei_catalog hashimyei_report_fragments lattice_catalog source_runtime_projection hero_mcp_surface
$(foreach t,$(HASHIMYEI),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/hashimyei)))

# ------------------------------ jkimyei --------------------------------------
JKIMYEI := jk_setup
$(foreach t,$(JKIMYEI),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/jkimyei)))

# ------------------------------ wikimyei/VICReg ------------------------------
VICREG := vicreg_4d_train vicreg_4d_projector_gpu_vs_cpu vicreg_4d_save_and_load vicreg_4d_phase1_stabilization
$(foreach t,$(VICREG),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/wikimyei)))

# ------------------------------ wikimyei/MDN ---------------------------------
MDN := mixture_density_network
$(foreach t,$(MDN),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/wikimyei)))

# ------------------------------ wikimyei/ExpectedValue -----------------------
EXPVAL := expected_value
$(foreach t,$(EXPVAL),$(eval $(call FORWARD_ONE,$(t),$(HERE_PATH)/bench/wikimyei)))

# 4) Shortcut groups (delegate only; no test logic changes)
.PHONY: parser dsl types exchange piaabo data iinuji iinuji_torch tsiemene iitepi https db hashimyei jkimyei vicreg mdn expval
parser: $(addprefix test_,$(BNF_PARSER_CORE))
dsl: $(addprefix test_,$(DSL))
types: $(addprefix test_,$(TYPES))
exchange: $(addprefix test_,$(EXCHANGE))
piaabo: $(addprefix test_,$(PIAABO))
data: $(addprefix test_,$(DATA))
iinuji: test_iinuji_plots test_iinuji_logs test_iinuji_suit test_iinuji_mouse test_iinuji_tsi
iinuji_torch: test_iinuji_toroid test_iinuji_data_viz test_iinuji_timeseries test_iinuji_cmd test_iinuji_cmd_home test_iinuji_cmd_canonical_paths test_iinuji_cmd_terminal
tsiemene: $(addprefix test_,$(TSIEMENE))
iitepi: $(addprefix test_,$(IITEPI))
https: $(addprefix test_,$(HTTPS))
db: $(addprefix test_,$(DB))
hashimyei: $(addprefix test_,$(HASHIMYEI))
jkimyei: $(addprefix test_,$(JKIMYEI))
vicreg: $(addprefix test_,$(VICREG))
mdn: $(addprefix test_,$(MDN))
expval: $(addprefix test_,$(EXPVAL))

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
	$(Q)$(CLEAN_RM_CMD) $(TEST_OUT)/test_*
