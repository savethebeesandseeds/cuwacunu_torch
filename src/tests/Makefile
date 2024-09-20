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
# USUAL_LINK_CMD = $(LOG_START) && $(GXX) $(GXX_FLAGS) $(USUAL_INCLUDE_PATHS) $(OBJECT_TO_LINK) $(USUAL_LD_FLAGS) $< -o $@ && $(LOG_END)
EXTENDED_LINK_CMD = $(LOG_START) && $(LOCAL_CLEAN) && $(GXX) $(GXX_FLAGS) $(USUAL_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS) $(OBJECT_TO_LINK) $(LIBCURl_LD_LAGS) $(SSL_LD_FLAGS) $(USUAL_LD_FLAGS) $< -o $@ && $(LOG_END)

# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_config test_config
$(TEST_BUILD)/test_config: $(HERE_PATH)/test_config.cpp
	@$(EXTENDED_LINK_CMD) 
	$(RUN_MODE) $(TEST_BUILD)/test_config
test_config: $(TEST_BUILD)/test_config
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_security test_security
$(TEST_BUILD)/test_security: $(HERE_PATH)/test_security.cpp
	@$(EXTENDED_LINK_CMD) 
	$(RUN_MODE) $(TEST_BUILD)/test_security
test_security: $(TEST_BUILD)/test_security
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_encryption test_encryption
$(TEST_BUILD)/test_encryption: $(HERE_PATH)/test_encryption.cpp
	@$(EXTENDED_LINK_CMD) 
	$(RUN_MODE) $(TEST_BUILD)/test_encryption
test_encryption: $(TEST_BUILD)/test_encryption
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_binance_serialization test_binance_serialization
$(TEST_BUILD)/test_binance_serialization: $(HERE_PATH)/test_binance_serialization.cpp
	@$(EXTENDED_LINK_CMD) 
	$(RUN_MODE) $(TEST_BUILD)/test_binance_serialization
test_binance_serialization: $(TEST_BUILD)/test_binance_serialization
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_binance_deserialization test_binance_deserialization
$(TEST_BUILD)/test_binance_deserialization: $(HERE_PATH)/test_binance_deserialization.cpp
	@$(EXTENDED_LINK_CMD) 
	$(RUN_MODE) $(TEST_BUILD)/test_binance_deserialization
test_binance_deserialization: $(TEST_BUILD)/test_binance_deserialization
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_websocket test_websocket
$(TEST_BUILD)/test_websocket: $(HERE_PATH)/test_websocket.cpp
	@$(EXTENDED_LINK_CMD) 
	$(RUN_MODE) $(TEST_BUILD)/test_websocket
test_websocket: $(TEST_BUILD)/test_websocket
# -----------------------------------------------------------------
# - - - General Target -- - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: all
all: $(TEST_BUILD)/test_config $(TEST_BUILD)/test_security $(TEST_BUILD)/test_encryption $(TEST_BUILD)/test_binance_serialization $(TEST_BUILD)/test_binance_deserialization $(TEST_BUILD)/test_websocket
	@$(LOG_SUCCESS)
.PHONY: clean_tests
clean_tests:
	@echo "$(COLOR_RED) Cleaning test build $(COLOR_RESET)"
	shred -u $(TEST_BUILD)/*.d $(TEST_BUILD)/*.o
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------