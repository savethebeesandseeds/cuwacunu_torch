# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------
ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config

# -----------------------------------------------------------------------------
# Local Paths
# -----------------------------------------------------------------------------
HERE_PATH  := $(TESTS_PATH)/bench/camahjucunu/exchange
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

# -----------------------------------------------------------------------------
# Build + run aliases
# -----------------------------------------------------------------------------
$(eval $(call TEST_ONEFILE,test_exchange_types_serialization,   test_exchange_types_serialization.cpp))
$(eval $(call TEST_ONEFILE,test_exchange_types_deserialization, test_exchange_types_deserialization.cpp))
$(eval $(call TEST_ONEFILE,test_binance_testnet,               test_binance_testnet.cpp))
$(eval $(call TEST_ONEFILE,test_binance_mech_server,           test_binance_mech_server.cpp))
$(eval $(call TEST_ONEFILE,test_binance_mech_data,             test_binance_mech_data.cpp))
$(eval $(call TEST_ONEFILE,test_binance_mech_account,          test_binance_mech_account.cpp))
$(eval $(call TEST_ONEFILE,test_binance_mech_trade,            test_binance_mech_trade.cpp))

# ---- Headers for compile -----------------------------------------------------
$(TEST_OUT)/test_exchange_types_serialization:    INCLUDES_EXTRA += $(SSL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS)
$(TEST_OUT)/test_exchange_types_deserialization:  INCLUDES_EXTRA += $(SSL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS)
$(TEST_OUT)/test_binance_testnet:                 INCLUDES_EXTRA += $(SSL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS)
$(TEST_OUT)/test_binance_mech_server:             INCLUDES_EXTRA += $(SSL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS)
$(TEST_OUT)/test_binance_mech_data:               INCLUDES_EXTRA += $(SSL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS)
$(TEST_OUT)/test_binance_mech_account:            INCLUDES_EXTRA += $(SSL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS)
$(TEST_OUT)/test_binance_mech_trade:              INCLUDES_EXTRA += $(SSL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS)

# ---- Link order: consumer (curl) before provider (common) --------------------
# Disable the automatic default so we can order explicitly:
TEST_DEFAULT_LDLIBS :=
# Then add: curl → common (provider) → system libs
$(TEST_OUT)/test_exchange_types_serialization:    LDLIBS_EXTRA += $(libssl_a) $(libcurl_a) $(libcommon_a) $(LDLIBS_ssl) $(LDLIBS_curl)
$(TEST_OUT)/test_exchange_types_deserialization:  LDLIBS_EXTRA += $(libssl_a) $(libcurl_a) $(libcommon_a) $(LDLIBS_ssl) $(LDLIBS_curl)
$(TEST_OUT)/test_binance_testnet:                 LDLIBS_EXTRA += $(libssl_a) $(libcurl_a) $(libcommon_a) $(LDLIBS_ssl) $(LDLIBS_curl)
$(TEST_OUT)/test_binance_mech_server:             LDLIBS_EXTRA += $(libssl_a) $(libcurl_a) $(libcommon_a) $(LDLIBS_ssl) $(LDLIBS_curl)
$(TEST_OUT)/test_binance_mech_data:               LDLIBS_EXTRA += $(libssl_a) $(libcurl_a) $(libcommon_a) $(LDLIBS_ssl) $(LDLIBS_curl)
$(TEST_OUT)/test_binance_mech_account:            LDLIBS_EXTRA += $(libssl_a) $(libcurl_a) $(libcommon_a) $(LDLIBS_ssl) $(LDLIBS_curl)
$(TEST_OUT)/test_binance_mech_trade:              LDLIBS_EXTRA += $(libssl_a) $(libcurl_a) $(libcommon_a) $(LDLIBS_ssl) $(LDLIBS_curl)

# -----------------------------------------------------------------------------
# Aggregate build
# -----------------------------------------------------------------------------
.PHONY: all
all: \
	$(OUTPUT_PATH)/test_exchange_types_serialization \
	$(OUTPUT_PATH)/test_exchange_types_deserialization \
	$(OUTPUT_PATH)/test_binance_testnet \
	$(OUTPUT_PATH)/test_binance_mech_server \
	$(OUTPUT_PATH)/test_binance_mech_data \
	$(OUTPUT_PATH)/test_binance_mech_account \
	$(OUTPUT_PATH)/test_binance_mech_trade
	@$(LOG_SUCCESS)

# -----------------------------------------------------------------------------
# Aggregate run
# -----------------------------------------------------------------------------
.PHONY: run
run: \
     run-test_exchange_types_serialization \
     run-test_exchange_types_deserialization \
     run-test_binance_testnet \
     run-test_binance_mech_server \
     run-test_binance_mech_data \
     run-test_binance_mech_account \
     run-test_binance_mech_trade

# -----------------------------------------------------------------------------
# Clean
# -----------------------------------------------------------------------------
.PHONY: clean
clean:
	@rm -f $(OUTPUT_PATH)/test_exchange_* $(OUTPUT_PATH)/test_binance_*
