# =============================================================================
# tests/bench/camahjucunu/db/Makefile
# =============================================================================
# 0) Root & Config
ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config

# 1) Local Paths
HERE_PATH  := $(TESTS_PATH)/bench/camahjucunu/db
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

# 2) Defaults (libs/flags)
TEST_DEFAULT_LDLIBS :=

# 3) Tests (build + run aliases)
COMMON_LINK = $(libssl_a) $(libcommon_a) $(LDLIBS_ssl)
$(eval $(call TEST_ONEFILE, test_db,          test_db.cpp,          $(COMMON_LINK)))

# 4) Target-specific includes/link flags
$(TEST_OUT)/test_db%: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS)

# 5) Aggregate: all
.PHONY: all
all: \
	$(TEST_OUT)/test_db
	@$(LOG_SUCCESS)

# 6) Aggregate: run
.PHONY: run
run: \
	run-test_db

# 7) Clean
.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/test_db*
