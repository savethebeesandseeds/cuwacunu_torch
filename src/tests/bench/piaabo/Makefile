# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config

# -----------------------------------------------------------------------------
# Local Paths
# -----------------------------------------------------------------------------
HERE_PATH  := $(TESTS_PATH)/bench/piaabo
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

# -----------------------------------------------------------------------------
# Build + run aliases
# -----------------------------------------------------------------------------
$(eval $(call TEST_ONEFILE,test_darchitecture,        test_darchitecture.cpp))
$(eval $(call TEST_ONEFILE,test_dconfig,              test_dconfig.cpp))
$(eval $(call TEST_ONEFILE,test_dsecurity,            test_dsecurity.cpp))
$(eval $(call TEST_ONEFILE,test_dencryption,          test_dencryption.cpp))
$(eval $(call TEST_ONEFILE,test_djson_parsing,        test_djson_parsing.cpp))
$(eval $(call TEST_ONEFILE,test_dfiles_binary_to_vector, test_dfiles_binary_to_vector.cpp))

# ---- Attach per-test deps ---------------------------------------------------
# Plain (just common)
$(TEST_OUT)/test_darchitecture:        INCLUDES_EXTRA += $(USUAL_INCLUDE_PATHS)
$(TEST_OUT)/test_darchitecture:        LDLIBS_EXTRA   += $(libcommon_a)

$(TEST_OUT)/test_dconfig:              INCLUDES_EXTRA += $(USUAL_INCLUDE_PATHS)
$(TEST_OUT)/test_dconfig:              LDLIBS_EXTRA   += $(libcommon_a)

$(TEST_OUT)/test_djson_parsing:        INCLUDES_EXTRA += $(USUAL_INCLUDE_PATHS)
$(TEST_OUT)/test_djson_parsing:        LDLIBS_EXTRA   += $(libcommon_a)

# Needs SSL
$(TEST_OUT)/test_dsecurity:            INCLUDES_EXTRA += $(SSL_INCLUDE_PATHS)
$(TEST_OUT)/test_dsecurity:            LDLIBS_EXTRA   += $(libssl_a) $(libcommon_a) $(LDLIBS_ssl)

$(TEST_OUT)/test_dencryption:          INCLUDES_EXTRA += $(SSL_INCLUDE_PATHS)
$(TEST_OUT)/test_dencryption:          LDLIBS_EXTRA   += $(libssl_a) $(libcommon_a) $(LDLIBS_ssl)

# Needs SSL + cURL
$(TEST_OUT)/test_dfiles_binary_to_vector: INCLUDES_EXTRA += $(SSL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS)
$(TEST_OUT)/test_dfiles_binary_to_vector: LDLIBS_EXTRA   += $(libssl_a) $(libcurl_a) $(libcommon_a) $(LDLIBS_ssl) $(LDLIBS_curl)

# -----------------------------------------------------------------------------
# Aggregate build
# -----------------------------------------------------------------------------
.PHONY: all
all: \
	$(OUTPUT_PATH)/test_darchitecture \
	$(OUTPUT_PATH)/test_dconfig \
	$(OUTPUT_PATH)/test_dsecurity \
	$(OUTPUT_PATH)/test_dencryption \
	$(OUTPUT_PATH)/test_djson_parsing \
	$(OUTPUT_PATH)/test_dfiles_binary_to_vector
	$(MAKE) -C $(HERE_PATH)/torch_compat all
	@$(LOG_SUCCESS)

# -----------------------------------------------------------------------------
# Aggregate run
# -----------------------------------------------------------------------------
.PHONY: run
run: \
     run-test_darchitecture \
     run-test_dconfig \
     run-test_dsecurity \
     run-test_dencryption \
     run-test_djson_parsing \
     run-test_dfiles_binary_to_vector

# -----------------------------------------------------------------------------
# Clean
# -----------------------------------------------------------------------------
.PHONY: clean
clean:
	@rm -f $(OUTPUT_PATH)/test_darchitecture \
	       $(OUTPUT_PATH)/test_dconfig \
	       $(OUTPUT_PATH)/test_dsecurity \
	       $(OUTPUT_PATH)/test_dencryption \
	       $(OUTPUT_PATH)/test_djson_parsing \
	       $(OUTPUT_PATH)/test_dfiles_binary_to_vector
