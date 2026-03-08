# =============================================================================
# tests/bench/hashimyei/Makefile — hashimyei focused tests
# =============================================================================
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config

HERE_PATH  := $(TESTS_PATH)/bench/hashimyei
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

DB_LINK   = $(libssl_a) $(libcommon_a) $(LDLIBS_ssl)
DATA_LINK = $(libtorch_a) $(libssl_a) $(libcurl_a) $(libcommon_a) $(LDLIBS_torch) $(LDLIBS_ssl) $(LDLIBS_curl)

$(eval $(call TEST_ONEFILE, test_hashimyei_catalog, test_hashimyei_catalog.cpp, $(DB_LINK)))
$(eval $(call TEST_ONEFILE, test_hashimyei_artifacts, test_hashimyei_artifacts.cpp, $(DATA_LINK)))

$(TEST_OUT)/test_hashimyei_catalog: INCLUDES_EXTRA += $(SSL_INCLUDE_PATHS)
$(TEST_OUT)/test_hashimyei_artifacts: INCLUDES_EXTRA += $(SSL_INCLUDE_PATHS)

.PHONY: all
all: \
	$(TEST_OUT)/test_hashimyei_catalog \
	$(TEST_OUT)/test_hashimyei_artifacts
	@$(LOG_SUCCESS)

.PHONY: run
run: run-test_hashimyei_catalog run-test_hashimyei_artifacts

.PHONY: clean
clean:
	@rm -f \
	  $(TEST_OUT)/test_hashimyei_catalog \
	  $(TEST_OUT)/test_hashimyei_artifacts
