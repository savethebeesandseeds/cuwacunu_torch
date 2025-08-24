# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------
ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config

# -----------------------------------------------------------------------------
# Local Paths
# -----------------------------------------------------------------------------
HERE_PATH  := $(TESTS_PATH)/bench/camahjucunu/https
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

# -----------------------------------------------------------------------------
# Build + run aliases
# -----------------------------------------------------------------------------
$(eval $(call TEST_ONEFILE,test_websocket, test_websocket.cpp))

# ---- Attach SSL/cURL deps (headers + archives + system libs) ----------------
# Headers for compile:
$(TEST_OUT)/test_websocket: INCLUDES_EXTRA += $(SSL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS)

# Link order: consumer (curl) before provider (common) to satisfy static deps
TEST_DEFAULT_LDLIBS :=
$(TEST_OUT)/test_websocket: LDLIBS_EXTRA += $(libssl_a) $(libcurl_a) $(libcommon_a) $(LDLIBS_ssl) $(LDLIBS_curl)

# -----------------------------------------------------------------------------
# Aggregate build
# -----------------------------------------------------------------------------
.PHONY: all
all: \
	$(OUTPUT_PATH)/test_websocket
	@$(LOG_SUCCESS)

# -----------------------------------------------------------------------------
# Aggregate run
# -----------------------------------------------------------------------------
.PHONY: run
run: \
     run-test_websocket

# -----------------------------------------------------------------------------
# Clean
# -----------------------------------------------------------------------------
.PHONY: clean
clean:
	@rm -f $(OUTPUT_PATH)/test_websocket
