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
.PHONY: $(TEST_BUILD)/test_exchange_serialization test_exchange_serialization
$(TEST_BUILD)/test_exchange_serialization: $(HERE_PATH)/test_exchange_serialization.cpp
	@$(EXTENDED_LINK_CMD) 
	$(RUN_MODE) $(TEST_BUILD)/test_exchange_serialization
test_exchange_serialization: $(TEST_BUILD)/test_exchange_serialization
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_exchange_deserialization test_exchange_deserialization
$(TEST_BUILD)/test_exchange_deserialization: $(HERE_PATH)/test_exchange_deserialization.cpp
	@$(EXTENDED_LINK_CMD) > make.log
	$(RUN_MODE) $(TEST_BUILD)/test_exchange_deserialization
test_exchange_deserialization: $(TEST_BUILD)/test_exchange_deserialization
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
.PHONY: $(TEST_BUILD)/test_binance_mech test_binance_mech
$(TEST_BUILD)/test_binance_mech: $(HERE_PATH)/test_binance_mech.cpp
	@$(EXTENDED_LINK_CMD)
	$(RUN_MODE) $(TEST_BUILD)/test_binance_mech
test_binance_mech: $(TEST_BUILD)/test_binance_mech
# -----------------------------------------------------------------
# - - - General Target -- - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: all
all: $(TEST_BUILD)/test_exchange_serialization $(TEST_BUILD)/test_exchange_deserialization $(TEST_BUILD)/test_websocket $(TEST_BUILD)/test_binance_mech
	@$(LOG_SUCCESS)
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------