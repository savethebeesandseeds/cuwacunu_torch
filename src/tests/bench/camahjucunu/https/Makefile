# =============================================================================
# tests/bench/camahjucunu/https/Makefile — websocket tests
# =============================================================================
# 0) Root & Config
ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config

# 1) Local Paths
HERE_PATH  := $(TESTS_PATH)/bench/camahjucunu/https
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

# 2) Defaults (libs/flags)
TEST_DEFAULT_LDLIBS :=

# 3) Tests (build + run aliases)
$(eval $(call TEST_ONEFILE, test_websocket, test_websocket.cpp, $(libssl_a) $(libcurl_a) $(libcommon_a) $(LDLIBS_ssl) $(LDLIBS_curl)))

# 4) Target-specific includes/link flags
$(TEST_OUT)/test_websocket: INCLUDES_EXTRA += $(SSL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS)

# 5) Aggregate: all
.PHONY: all
all: $(TEST_OUT)/test_websocket
	@$(LOG_SUCCESS)

# 6) Aggregate: run
.PHONY: run
run: run-test_websocket

# 7) Clean
.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/test_websocket
