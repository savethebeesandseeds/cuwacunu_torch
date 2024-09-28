# -----------------------------------------------------------------
# Dependencies - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config
# -----------------------------------------------------------------
# Local Makefile Variables - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
HERE_PATH := $(ROOT_PATH)/tests/bench/piaabo
TEST_BUILD := $(ROOT_PATH)/tests/build
# USUAL_LINK_CMD = $(LOG_START) && $(GXX) $(GXX_FLAGS) $(USUAL_INCLUDE_PATHS) $(OBJECT_TO_LINK) $(USUAL_LD_FLAGS) $< -o $@ && $(LOG_END)
EXTENDED_LINK_CMD = $(LOG_START) && $(LOCAL_CLEAN) && $(GXX) $(GXX_FLAGS) $(USUAL_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS) $(OBJECT_TO_LINK) $(LIBCURl_LD_LAGS) $(SSL_LD_FLAGS) $(USUAL_LD_FLAGS) $< -o $@ && $(LOG_END)
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_darchitecture test_darchitecture
$(TEST_BUILD)/test_darchitecture: $(HERE_PATH)/test_darchitecture.cpp
	@$(EXTENDED_LINK_CMD) 
	$(RUN_MODE) $(TEST_BUILD)/test_darchitecture
test_darchitecture: $(TEST_BUILD)/test_darchitecture
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_dconfig test_dconfig
$(TEST_BUILD)/test_dconfig: $(HERE_PATH)/test_dconfig.cpp
	@$(EXTENDED_LINK_CMD) 
	$(RUN_MODE) $(TEST_BUILD)/test_dconfig
test_dconfig: $(TEST_BUILD)/test_dconfig
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_dsecurity test_dsecurity
$(TEST_BUILD)/test_dsecurity: $(HERE_PATH)/test_dsecurity.cpp
	@$(EXTENDED_LINK_CMD) 
	$(RUN_MODE) $(TEST_BUILD)/test_dsecurity
test_dsecurity: $(TEST_BUILD)/test_dsecurity
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_dencryption test_dencryption
$(TEST_BUILD)/test_dencryption: $(HERE_PATH)/test_dencryption.cpp
	@$(EXTENDED_LINK_CMD)
	$(RUN_MODE) $(TEST_BUILD)/test_dencryption
test_dencryption: $(TEST_BUILD)/test_dencryption
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_json_parsing test_json_parsing
$(TEST_BUILD)/test_json_parsing: $(HERE_PATH)/test_json_parsing.cpp
	@$(EXTENDED_LINK_CMD)
	$(RUN_MODE) $(TEST_BUILD)/test_json_parsing
test_json_parsing: $(TEST_BUILD)/test_json_parsing
# -----------------------------------------------------------------
# - - - General Target -- - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: all
all: $(TEST_BUILD)/test_dconfig $(TEST_BUILD)/test_dsecurity $(TEST_BUILD)/test_dencryption 
	@$(LOG_SUCCESS)
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------