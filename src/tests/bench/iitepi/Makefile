# =============================================================================
# tests/bench/iitepi/Makefile — iitepi demonstrations/tests
# =============================================================================
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config

HERE_PATH  := $(TESTS_PATH)/bench/iitepi/
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

TEST_DEFAULT_LDLIBS :=

COMMON_LINK = $(libtorch_a) $(libcurl_a) $(libssl_a) $(libcommon_a) $(LDLIBS_torch) $(LDLIBS_curl) $(LDLIBS_ssl)
$(eval $(call TEST_ONEFILE, test_iitepi_board, test_iitepi_board.cpp, $(COMMON_LINK)))

$(TEST_OUT)/test_iitepi_%: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS)

.PHONY: all
all: \
		$(TEST_OUT)/test_iitepi_board
	@$(LOG_SUCCESS)

.PHONY: run
run: run-test_iitepi_board

.PHONY: clean
clean:
	@rm -f \
	  $(TEST_OUT)/test_iitepi_board
