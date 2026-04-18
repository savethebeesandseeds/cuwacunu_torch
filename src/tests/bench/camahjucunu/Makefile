# =============================================================================
# tests/bench/camahjucunu/Makefile — flattened camahjucunu bench tests
# =============================================================================
# 0) Root & Config
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config

# 1) Local Paths
HERE_PATH  := $(TESTS_PATH)/bench/camahjucunu
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

# 2) Defaults (libs/flags)
# (use defaults from Makefile.config)

# 3) Tests (build + run aliases)
DATA_LINK      = $(libtorch_a) $(libssl_a) $(libcurl_a) $(libcommon_a) $(LDLIBS_torch) $(LDLIBS_ssl) $(LDLIBS_curl)
EXCHANGE_LINK  = $(libssl_a) $(libcurl_a) $(libcommon_a) $(LDLIBS_ssl) $(LDLIBS_curl)
TYPES_LINK     = $(libcurl_a) $(libssl_a) $(libcommon_a) $(LDLIBS_ssl) $(LDLIBS_curl)
DB_LINK        = $(libssl_a) $(libcommon_a) $(LDLIBS_ssl)
WEBSOCKET_LINK = $(libssl_a) $(libcurl_a) $(libcommon_a) $(LDLIBS_ssl) $(LDLIBS_curl)
DSL_RUNTIME_LINK = $(libtorch_a) $(libcurl_a) $(libssl_a) $(libcommon_a) $(LDLIBS_torch) $(LDLIBS_curl) $(LDLIBS_ssl)

$(eval $(call TEST_ONEFILE, test_dsl_observation, test_dsl_observation.cpp, $(DSL_RUNTIME_LINK)))
$(eval $(call TEST_ONEFILE, test_dsl_latent_lineage_state,            test_dsl_latent_lineage_state.cpp))
$(eval $(call TEST_ONEFILE, test_dsl_jkimyei_specs,        test_dsl_jkimyei_specs.cpp, $(DSL_RUNTIME_LINK)))
$(eval $(call TEST_ONEFILE, test_dsl_iitepi_campaign,      test_dsl_iitepi_campaign.cpp, $(DSL_RUNTIME_LINK)))
$(eval $(call TEST_ONEFILE, test_wave_contract_binding_variables, test_wave_contract_binding_variables.cpp, $(DSL_RUNTIME_LINK)))
$(eval $(call TEST_ONEFILE, test_dsl_tsiemene_circuit,     test_dsl_tsiemene_circuit.cpp, $(DSL_RUNTIME_LINK)))
$(eval $(call TEST_ONEFILE, test_dsl_iitepi_wave,        test_dsl_iitepi_wave.cpp, $(DSL_RUNTIME_LINK)))
$(eval $(call TEST_ONEFILE, test_dsl_canonical_path,       test_dsl_canonical_path.cpp, $(DSL_RUNTIME_LINK)))

$(eval $(call TEST_ONEFILE, test_memory_mapped_datafile,          test_memory_mapped_datafile.cpp,          $(DATA_LINK)))
$(eval $(call TEST_ONEFILE, test_memory_mapped_dataset,           test_memory_mapped_dataset.cpp,           $(DATA_LINK)))
$(eval $(call TEST_ONEFILE, test_memory_mapped_concat_dataset,    test_memory_mapped_concat_dataset.cpp,    $(DATA_LINK)))
$(eval $(call TEST_ONEFILE, test_memory_mapped_concat_dataloader, test_memory_mapped_concat_dataloader.cpp, $(DATA_LINK)))
$(eval $(call TEST_ONEFILE, test_memory_mapped_edge_cases,        test_memory_mapped_edge_cases.cpp,        $(DATA_LINK)))

$(eval $(call TEST_ONEFILE, test_db,     test_db.cpp,     $(DB_LINK)))
$(eval $(call TEST_ONEFILE, test_db_rag, test_db_rag.cpp, $(DB_LINK)))
$(eval $(call TEST_ONEFILE, test_db_query_utils, test_db_query_utils.cpp, $(DB_LINK)))

$(eval $(call TEST_ONEFILE, test_binance_testnet,      test_binance_testnet.cpp,      $(EXCHANGE_LINK)))
$(eval $(call TEST_ONEFILE, test_binance_mech_server,  test_binance_mech_server.cpp,  $(EXCHANGE_LINK)))
$(eval $(call TEST_ONEFILE, test_binance_mech_data,    test_binance_mech_data.cpp,    $(EXCHANGE_LINK)))
$(eval $(call TEST_ONEFILE, test_binance_mech_account, test_binance_mech_account.cpp, $(EXCHANGE_LINK)))
$(eval $(call TEST_ONEFILE, test_binance_mech_trade,   test_binance_mech_trade.cpp,   $(EXCHANGE_LINK)))

$(eval $(call TEST_ONEFILE, test_types_serialization,   test_types_serialization.cpp,   $(TYPES_LINK)))
$(eval $(call TEST_ONEFILE, test_types_deserialization, test_types_deserialization.cpp, $(TYPES_LINK)))

$(eval $(call TEST_ONEFILE, test_websocket, test_websocket.cpp, $(WEBSOCKET_LINK)))

# 4) Target-specific includes/link flags
$(TEST_OUT)/test_memory_mapped_%: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS)
$(TEST_OUT)/test_db%: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS)
$(TEST_OUT)/test_binance_%: INCLUDES_EXTRA += $(SSL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS)
$(TEST_OUT)/test_types_%: INCLUDES_EXTRA += $(SSL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS)
$(TEST_OUT)/test_websocket: INCLUDES_EXTRA += $(SSL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS)

# 5) Aggregate: all
.PHONY: all
all: \
	$(TEST_OUT)/test_dsl_observation \
	$(TEST_OUT)/test_dsl_latent_lineage_state \
	$(TEST_OUT)/test_dsl_jkimyei_specs \
	$(TEST_OUT)/test_dsl_iitepi_campaign \
	$(TEST_OUT)/test_wave_contract_binding_variables \
	$(TEST_OUT)/test_dsl_tsiemene_circuit \
	$(TEST_OUT)/test_dsl_iitepi_wave \
	$(TEST_OUT)/test_dsl_canonical_path \
	$(TEST_OUT)/test_memory_mapped_datafile \
	$(TEST_OUT)/test_memory_mapped_dataset \
	$(TEST_OUT)/test_memory_mapped_concat_dataset \
	$(TEST_OUT)/test_memory_mapped_concat_dataloader \
	$(TEST_OUT)/test_memory_mapped_edge_cases \
	$(TEST_OUT)/test_db \
	$(TEST_OUT)/test_db_rag \
	$(TEST_OUT)/test_db_query_utils \
	$(TEST_OUT)/test_binance_testnet \
	$(TEST_OUT)/test_binance_mech_server \
	$(TEST_OUT)/test_binance_mech_data \
	$(TEST_OUT)/test_binance_mech_account \
	$(TEST_OUT)/test_binance_mech_trade \
	$(TEST_OUT)/test_types_serialization \
	$(TEST_OUT)/test_types_deserialization \
	$(TEST_OUT)/test_websocket
	@$(LOG_SUCCESS)

# 6) Aggregate: run
.PHONY: run
run: \
	run-test_dsl_observation \
	run-test_dsl_latent_lineage_state \
	run-test_dsl_jkimyei_specs \
	run-test_dsl_iitepi_campaign \
	run-test_wave_contract_binding_variables \
	run-test_dsl_tsiemene_circuit \
	run-test_dsl_iitepi_wave \
	run-test_dsl_canonical_path \
	run-test_memory_mapped_datafile \
	run-test_memory_mapped_dataset \
	run-test_memory_mapped_concat_dataset \
	run-test_memory_mapped_concat_dataloader \
	run-test_memory_mapped_edge_cases \
	run-test_db \
	run-test_db_rag \
	run-test_binance_testnet \
	run-test_binance_mech_server \
	run-test_binance_mech_data \
	run-test_binance_mech_account \
	run-test_binance_mech_trade \
	run-test_types_serialization \
	run-test_types_deserialization \
	run-test_websocket

# 7) Clean
.PHONY: clean
clean:
	$(Q)$(CLEAN_RM_CMD) \
	  $(TEST_OUT)/test_dsl_* \
	  $(TEST_OUT)/test_memory_mapped_* \
	  $(TEST_OUT)/test_db* \
	  $(TEST_OUT)/test_binance_* \
	  $(TEST_OUT)/test_types_* \
	  $(TEST_OUT)/test_websocket
