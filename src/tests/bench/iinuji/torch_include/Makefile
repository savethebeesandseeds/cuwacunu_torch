# =============================================================================
# tests/bench/iinuji/torch_include/Makefile — ncurses + torch TUI tests
# =============================================================================

# 0) Root & Config
ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config

# 1) Local Paths
HERE_PATH  := $(TESTS_PATH)/bench/iinuji/torch_include
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

# 2) Defaults (libs/flags)
TEST_DEFAULT_LDLIBS :=

# 3) Tests (build + run aliases)
COMMON_LINK = \
  $(libtorch_a) $(libcurl_a) $(libssl_a) $(libcommon_a) \
  $(LDLIBS_torch) $(LDLIBS_curl) $(LDLIBS_ssl) $(LDLIBS_ncurses)

$(eval $(call TEST_ONEFILE, test_iinuji_data_viz, test_iinuji_data_viz.cpp, $(COMMON_LINK)))
$(eval $(call TEST_ONEFILE, test_iinuji_timeseries, test_iinuji_timeseries.cpp, $(COMMON_LINK)))

# 4) Target-specific includes
$(TEST_OUT)/test_iinuji_data_viz: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS) $(NCURSES_INCLUDE_PATHS)
$(TEST_OUT)/test_iinuji_timeseries: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS) $(NCURSES_INCLUDE_PATHS)

# 5) Aggregate: all
.PHONY: all
all: 
	$(TEST_OUT)/test_iinuji_data_viz
	$(TEST_OUT)/test_iinuji_timeseries
	@$(LOG_SUCCESS)


# 6) Aggregate: run
.PHONY: run
run: run-test_iinuji_data_viz run-test_iinuji_timeseries

# 7) Clean
.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/test_iinuji_data_viz $(TEST_OUT)/test_iinuji_timeseries
