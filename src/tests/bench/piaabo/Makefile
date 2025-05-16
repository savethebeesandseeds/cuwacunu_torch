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
COMMON_COMPILE_CMD = $(LOG_START) && $(LOCAL_CLEAN) && $(GXX) $(GXX_FLAGS) $(USUAL_INCLUDE_PATHS) $(OUTPUT_PATH)/common/*.o $(USUAL_LD_FLAGS) $< -o $@ && $(LOG_END)
COMMON_SSL_COMPILE_CMD = $(LOG_START) && $(LOCAL_CLEAN) && $(GXX) $(GXX_FLAGS) $(USUAL_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS) $(OUTPUT_PATH)/common/*.o $(OUTPUT_PATH)/openssl/*.o $(SSL_LD_FLAGS) $(USUAL_LD_FLAGS) $< -o $@ && $(LOG_END)
COMMON_SSL_CURL_COMPILE_CMD = $(LOG_START) && $(LOCAL_CLEAN) && $(GXX) $(GXX_FLAGS) $(USUAL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS) $(OUTPUT_PATH)/common/*.o $(OUTPUT_PATH)/openssl/*.o $(OUTPUT_PATH)/libcurl/*.o $(LIBCURL_LD_LAGS) $(SSL_LD_FLAGS) $(USUAL_LD_FLAGS) $< -o $@ && $(LOG_END)
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_darchitecture test_darchitecture
$(TEST_BUILD)/test_darchitecture: $(HERE_PATH)/test_darchitecture.cpp
	@$(COMMON_COMPILE_CMD) 
	$(RUN_MODE) $(TEST_BUILD)/test_darchitecture
test_darchitecture: $(TEST_BUILD)/test_darchitecture
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_dconfig test_dconfig
$(TEST_BUILD)/test_dconfig: $(HERE_PATH)/test_dconfig.cpp
	@$(COMMON_COMPILE_CMD) 
	$(RUN_MODE) $(TEST_BUILD)/test_dconfig
test_dconfig: $(TEST_BUILD)/test_dconfig
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_dsecurity test_dsecurity
$(TEST_BUILD)/test_dsecurity: $(HERE_PATH)/test_dsecurity.cpp
	@$(COMMON_SSL_COMPILE_CMD) 
	$(RUN_MODE) $(TEST_BUILD)/test_dsecurity
test_dsecurity: $(TEST_BUILD)/test_dsecurity
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_dencryption test_dencryption
$(TEST_BUILD)/test_dencryption: $(HERE_PATH)/test_dencryption.cpp
	@$(COMMON_SSL_COMPILE_CMD)
	$(RUN_MODE) $(TEST_BUILD)/test_dencryption
test_dencryption: $(TEST_BUILD)/test_dencryption
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_djson_parsing test_djson_parsing
$(TEST_BUILD)/test_djson_parsing: $(HERE_PATH)/test_djson_parsing.cpp
	@$(COMMON_COMPILE_CMD)
	$(RUN_MODE) $(TEST_BUILD)/test_djson_parsing
test_djson_parsing: $(TEST_BUILD)/test_djson_parsing
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_dfiles_binary_to_vector test_dfiles_binary_to_vector
$(TEST_BUILD)/test_dfiles_binary_to_vector: $(HERE_PATH)/test_dfiles_binary_to_vector.cpp
	@$(COMMON_SSL_CURL_COMPILE_CMD)
	$(RUN_MODE) $(TEST_BUILD)/test_dfiles_binary_to_vector
test_dfiles_binary_to_vector: $(TEST_BUILD)/test_dfiles_binary_to_vector
# -----------------------------------------------------------------
# - - - General Target -- - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: all
all: $(TEST_BUILD)/test_dconfig $(TEST_BUILD)/test_dsecurity $(TEST_BUILD)/test_dencryption $(TEST_BUILD)/test_darchitecture $(TEST_BUILD)/test_djson_parsing
	$(MAKE) -C $(HERE_PATH)/torch_compat all
	@$(LOG_SUCCESS)
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------