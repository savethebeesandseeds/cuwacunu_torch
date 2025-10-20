# =============================================================================
# tests/bench/camahjucunu/types/Makefile — type (de)serialization tests
# =============================================================================
# 0) Root & Config
ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config

# 1) Local Paths
HERE_PATH  := $(TESTS_PATH)/bench/camahjucunu/types
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

# 2) Defaults (libs/flags)
TEST_DEFAULT_LDLIBS :=

# 3) Tests (build + run aliases)
COMMON_LINK = $(libssl_a) $(libcurl_a) $(libcommon_a) $(LDLIBS_ssl) $(LDLIBS_curl)
$(eval $(call TEST_ONEFILE, test_types_serialization,   test_types_serialization.cpp,   $(COMMON_LINK)))
$(eval $(call TEST_ONEFILE, test_types_deserialization, test_types_deserialization.cpp, $(COMMON_LINK)))

# 4) Target-specific includes/link flags
$(TEST_OUT)/test_types_serialization:   INCLUDES_EXTRA += $(SSL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS)
$(TEST_OUT)/test_types_deserialization: INCLUDES_EXTRA += $(SSL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS)

# 5) Aggregate: all
.PHONY: all
all: \
	$(TEST_OUT)/test_types_serialization \
	$(TEST_OUT)/test_types_deserialization
	@$(LOG_SUCCESS)

# 6) Aggregate: run
.PHONY: run
run: \
	run-test_types_serialization \
	run-test_types_deserialization

# 7) Clean
.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/test_types_serialization $(TEST_OUT)/test_types_deserialization
