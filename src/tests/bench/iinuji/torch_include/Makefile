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
$(eval $(call TEST_ONEFILE, test_iinuji_cmd, test_iinuji_cmd.cpp, $(COMMON_LINK)))
$(eval $(call TEST_ONEFILE, test_iinuji_cmd_home, test_iinuji_cmd_home.cpp, $(COMMON_LINK)))
$(eval $(call TEST_ONEFILE, test_iinuji_cmd_canonical_paths, test_iinuji_cmd_canonical_paths.cpp, $(COMMON_LINK)))
$(eval $(call TEST_ONEFILE, test_iinuji_cmd_terminal, test_iinuji_cmd_terminal.cpp, $(COMMON_LINK)))
$(eval $(call TEST_ONEFILE, test_iinuji_board_completion, test_iinuji_board_completion.cpp, $(COMMON_LINK)))
$(eval $(call TEST_ONEFILE, test_iinuji_board_modes, test_iinuji_board_modes.cpp, $(COMMON_LINK)))

# 4) Target-specific includes
$(TEST_OUT)/test_iinuji_data_viz: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS) $(NCURSES_INCLUDE_PATHS)
$(TEST_OUT)/test_iinuji_timeseries: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS) $(NCURSES_INCLUDE_PATHS)
$(TEST_OUT)/test_iinuji_cmd: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS) $(NCURSES_INCLUDE_PATHS)
$(TEST_OUT)/test_iinuji_cmd_home: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS) $(NCURSES_INCLUDE_PATHS)
$(TEST_OUT)/test_iinuji_cmd_canonical_paths: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS) $(NCURSES_INCLUDE_PATHS)
$(TEST_OUT)/test_iinuji_cmd_terminal: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS) $(NCURSES_INCLUDE_PATHS)
$(TEST_OUT)/test_iinuji_board_completion: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS) $(NCURSES_INCLUDE_PATHS)
$(TEST_OUT)/test_iinuji_board_modes: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS) $(NCURSES_INCLUDE_PATHS)

# 5) Aggregate: all
.PHONY: all
all: \
	$(TEST_OUT)/test_iinuji_data_viz \
	$(TEST_OUT)/test_iinuji_timeseries \
	$(TEST_OUT)/test_iinuji_cmd \
	$(TEST_OUT)/test_iinuji_cmd_home \
	$(TEST_OUT)/test_iinuji_cmd_canonical_paths \
	$(TEST_OUT)/test_iinuji_cmd_terminal \
	$(TEST_OUT)/test_iinuji_board_completion \
	$(TEST_OUT)/test_iinuji_board_modes
	@$(LOG_SUCCESS)


# 6) Aggregate: run
.PHONY: run
run: run-test_iinuji_data_viz run-test_iinuji_timeseries run-test_iinuji_cmd run-test_iinuji_cmd_terminal run-test_iinuji_cmd_canonical_paths
run: run-test_iinuji_board_completion
run: run-test_iinuji_board_modes
run: run-test_iinuji_cmd_home

# 7) Clean
.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/test_iinuji_data_viz $(TEST_OUT)/test_iinuji_timeseries $(TEST_OUT)/test_iinuji_cmd $(TEST_OUT)/test_iinuji_cmd_home $(TEST_OUT)/test_iinuji_cmd_canonical_paths $(TEST_OUT)/test_iinuji_cmd_terminal $(TEST_OUT)/test_iinuji_board_completion $(TEST_OUT)/test_iinuji_board_modes
