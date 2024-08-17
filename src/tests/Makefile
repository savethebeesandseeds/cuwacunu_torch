# -----------------------------------------------------------------
# Dependencies - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
ROOT_PATH := ..
include $(ROOT_PATH)/Makefile.config
# -----------------------------------------------------------------
# Local Makefile Variables - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
HERE_PATH := $(ROOT_PATH)/tests
# USUAL_LINK_CMD = $(LOG_START) && $(GXX) $(GXX_FLAGS) $(USUAL_INCLUDE_PATHS) $(OBJECT_TO_LINK) $(USUAL_LD_FLAGS) $< -o $@ && $(LOG_END)
ENCRYPT_LINK_CMD = $(LOG_START) && $(LOCAL_CLEAN) && $(GXX) $(GXX_FLAGS) $(USUAL_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS) $(OBJECT_TO_LINK) $(SSL_LD_FLAGS) $(USUAL_LD_FLAGS) $< -o $@ && $(LOG_END)

# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: test_config
test_config: $(HERE_PATH)/test_config.cpp
	@$(ENCRYPT_LINK_CMD) && $(HERE_PATH)/test_config
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: test_security
test_security: $(HERE_PATH)/test_security.cpp
	@$(ENCRYPT_LINK_CMD) && $(HERE_PATH)/test_security
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: test_encryption
test_encryption: $(HERE_PATH)/test_encryption.cpp
	@$(ENCRYPT_LINK_CMD) && $(HERE_PATH)/test_encryption
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: test_binance_serialization
test_binance_serialization: $(HERE_PATH)/test_binance_serialization.cpp
	$(ENCRYPT_LINK_CMD) && $(HERE_PATH)/test_binance_serialization
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: test_binance_deserialization
test_binance_deserialization: $(HERE_PATH)/test_binance_deserialization.cpp
	$(ENCRYPT_LINK_CMD) && $(HERE_PATH)/test_binance_deserialization
# -----------------------------------------------------------------
# - - - General Target -- - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: all
all: test_config test_security test_encryption test_binance_serialization test_binance_deserialization
	@$(LOG_SUCCESS)
.PHONY: clean_tests
clean_tests:
	@echo "$(COLOR_RED) Cleaning test build $(COLOR_RESET)"
	shred -u $(HERE_PATH)/*.d $(HERE_PATH)/*.o
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------