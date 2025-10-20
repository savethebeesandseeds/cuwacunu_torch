# =============================================================================
# tests/bench/camahjucunu/exchange/Makefile — Binance exchange tests
# =============================================================================
# 0) Root & Config
ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config

# 1) Local Paths
HERE_PATH  := $(TESTS_PATH)/bench/camahjucunu/exchange
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

# 2) Defaults (libs/flags)
TEST_DEFAULT_LDLIBS :=

# 3) Tests (build + run aliases)
COMMON_LINK = $(libssl_a) $(libcurl_a) $(libcommon_a) $(LDLIBS_ssl) $(LDLIBS_curl)
$(eval $(call TEST_ONEFILE, test_binance_testnet,      test_binance_testnet.cpp,      $(COMMON_LINK)))
$(eval $(call TEST_ONEFILE, test_binance_mech_server,  test_binance_mech_server.cpp,  $(COMMON_LINK)))
$(eval $(call TEST_ONEFILE, test_binance_mech_data,    test_binance_mech_data.cpp,    $(COMMON_LINK)))
$(eval $(call TEST_ONEFILE, test_binance_mech_account, test_binance_mech_account.cpp, $(COMMON_LINK)))
$(eval $(call TEST_ONEFILE, test_binance_mech_trade,   test_binance_mech_trade.cpp,   $(COMMON_LINK)))

# 4) Target-specific includes/link flags
$(TEST_OUT)/test_binance_%: INCLUDES_EXTRA += $(SSL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS)

# 5) Aggregate: all
.PHONY: all
all: \
	$(TEST_OUT)/test_binance_testnet \
	$(TEST_OUT)/test_binance_mech_server \
	$(TEST_OUT)/test_binance_mech_data \
	$(TEST_OUT)/test_binance_mech_account \
	$(TEST_OUT)/test_binance_mech_trade
	@$(LOG_SUCCESS)

# 6) Aggregate: run
.PHONY: run
run: \
	run-test_binance_testnet \
	run-test_binance_mech_server \
	run-test_binance_mech_data \
	run-test_binance_mech_account \
	run-test_binance_mech_trade

# 7) Clean
.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/test_exchange_* $(TEST_OUT)/test_binance_*
