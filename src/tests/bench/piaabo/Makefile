# =============================================================================
# tests/bench/piaabo/Makefile — PIAABO tests + torch_compat aggregator
# =============================================================================
# 0) Root & Config
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config

# 1) Local Paths
HERE_PATH  := $(TESTS_PATH)/bench/piaabo
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

# 2) Defaults (libs/flags)
TEST_DEFAULT_LDLIBS :=

# 3) Tests (build + run aliases)
$(eval $(call TEST_ONEFILE, test_dconfig,                 test_dconfig.cpp,                 $(libcommon_a)))
$(eval $(call TEST_ONEFILE, test_dconfig_contract_lock,   test_dconfig_contract_lock.cpp,   $(libssl_a) $(libcommon_a) $(LDLIBS_ssl)))
$(eval $(call TEST_ONEFILE, test_dsecurity,               test_dsecurity.cpp,               $(libssl_a) $(libcommon_a) $(LDLIBS_ssl)))
$(eval $(call TEST_ONEFILE, test_dencryption,             test_dencryption.cpp,             $(libssl_a) $(libcommon_a) $(LDLIBS_ssl)))
$(eval $(call TEST_ONEFILE, test_djson_parsing,           test_djson_parsing.cpp,           $(libcommon_a)))
$(eval $(call TEST_ONEFILE, test_dfiles_binary_to_vector, test_dfiles_binary_to_vector.cpp, $(libcommon_a)))
$(eval $(call TEST_ONEFILE, test_bnf_grammar_lexer,      test_bnf_grammar_lexer.cpp,      $(libcommon_a)))
$(eval $(call TEST_ONEFILE, test_bnf_grammar_parser,     test_bnf_grammar_parser.cpp,     $(libcommon_a)))
$(eval $(call TEST_ONEFILE, test_bnf_instruction_lexer,  test_bnf_instruction_lexer.cpp,  $(libcommon_a)))
$(eval $(call TEST_ONEFILE, test_bnf_instruction_parser, test_bnf_instruction_parser.cpp, $(libcommon_a)))

# 4) Target-specific includes/link flags
$(TEST_OUT)/test_dsecurity:               INCLUDES_EXTRA += $(SSL_INCLUDE_PATHS)
$(TEST_OUT)/test_dconfig_contract_lock:   INCLUDES_EXTRA += $(SSL_INCLUDE_PATHS)
$(TEST_OUT)/test_dencryption:             INCLUDES_EXTRA += $(SSL_INCLUDE_PATHS)

# 5) Aggregate: all
.PHONY: all
all: \
	$(TEST_OUT)/test_dconfig \
	$(TEST_OUT)/test_dconfig_contract_lock \
	$(TEST_OUT)/test_dsecurity \
	$(TEST_OUT)/test_dencryption \
	$(TEST_OUT)/test_djson_parsing \
	$(TEST_OUT)/test_dfiles_binary_to_vector \
	$(TEST_OUT)/test_bnf_grammar_lexer \
	$(TEST_OUT)/test_bnf_grammar_parser \
	$(TEST_OUT)/test_bnf_instruction_lexer \
	$(TEST_OUT)/test_bnf_instruction_parser
	+$(MAKE) -C $(HERE_PATH)/torch_compat all
	@$(LOG_SUCCESS)

# 6) Aggregate: run
.PHONY: run
run: \
	run-test_dconfig \
	run-test_dconfig_contract_lock \
	run-test_dsecurity \
	run-test_dencryption \
	run-test_djson_parsing \
	run-test_dfiles_binary_to_vector \
	run-test_bnf_grammar_lexer \
	run-test_bnf_grammar_parser \
	run-test_bnf_instruction_lexer \
	run-test_bnf_instruction_parser

# 7) Clean
.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/test_dconfig \
	       $(TEST_OUT)/test_dconfig_contract_lock \
	       $(TEST_OUT)/test_dsecurity \
	       $(TEST_OUT)/test_dencryption \
	       $(TEST_OUT)/test_djson_parsing \
	       $(TEST_OUT)/test_dfiles_binary_to_vector \
	       $(TEST_OUT)/test_bnf_grammar_lexer \
	       $(TEST_OUT)/test_bnf_grammar_parser \
	       $(TEST_OUT)/test_bnf_instruction_lexer \
	       $(TEST_OUT)/test_bnf_instruction_parser
