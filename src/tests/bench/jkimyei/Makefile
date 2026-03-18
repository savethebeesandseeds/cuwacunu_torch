# =============================================================================
# tests/bench/jkimyei/Makefile — jkimyei bench tests
# =============================================================================
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config

HERE_PATH  := $(TESTS_PATH)/bench/jkimyei
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

TEST_DEFAULT_LDLIBS :=
COMMON_LINK = $(libtorch_a) $(libcurl_a) $(libssl_a) $(libcommon_a) $(LDLIBS_torch) $(LDLIBS_curl) $(LDLIBS_ssl)

$(eval $(call TEST_ONEFILE, test_jk_setup, test_jk_setup.cpp, $(COMMON_LINK)))
$(eval $(call TEST_ONEFILE, test_wave_contract_binding_runtime_snapshot, test_wave_contract_binding_runtime_snapshot.cpp, $(COMMON_LINK)))

$(TEST_OUT)/test_jk_setup: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS)
$(TEST_OUT)/test_wave_contract_binding_runtime_snapshot: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS)

.PHONY: all
all: $(TEST_OUT)/test_jk_setup $(TEST_OUT)/test_wave_contract_binding_runtime_snapshot
	@$(LOG_SUCCESS)

.PHONY: run
run: run-test_jk_setup run-test_wave_contract_binding_runtime_snapshot

.PHONY: clean
clean:
	$(Q)$(CLEAN_RM_CMD) $(TEST_OUT)/test_jk_setup $(TEST_OUT)/test_wave_contract_binding_runtime_snapshot
