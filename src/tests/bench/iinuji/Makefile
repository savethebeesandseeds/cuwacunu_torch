# =============================================================================
# tests/bench/iinuji/Makefile — ncurses TUI tests
# =============================================================================
# 0) Root & Config
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config

# 1) Local Paths
HERE_PATH  := $(TESTS_PATH)/bench/iinuji
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

# 2) Defaults (libs/flags)
# (use defaults from Makefile.config)

# 3) Tests (build + run aliases)
$(eval $(call TEST_ONEFILE, test_iinuji_plots,    		test_iinuji_plots.cpp))
$(eval $(call TEST_ONEFILE, test_iinuji_logs,     		test_iinuji_logs.cpp))
$(eval $(call TEST_ONEFILE, test_iinuji_suit,     		test_iinuji_suit.cpp))
$(eval $(call TEST_ONEFILE, test_iinuji_mouse,    		test_iinuji_mouse.cpp))
$(eval $(call TEST_ONEFILE, test_iinuji_toroid,   		test_iinuji_toroid.cpp))
$(eval $(call TEST_ONEFILE, test_iinuji_tsi,          test_iinuji_tsi.cpp))

# 4) Target-specific includes/link flags
$(TEST_OUT)/test_iinuji_%: INCLUDES_EXTRA += $(NCURSES_INCLUDE_PATHS)
$(TEST_OUT)/test_iinuji_%: LDLIBS_EXTRA   += $(LDLIBS_ncurses)

# torch-backed cmd screen lives in torch_include
.PHONY: run-test_iinuji_cmd
run-test_iinuji_cmd:
	@$(MAKE) -C $(HERE_PATH)/torch_include run-test_iinuji_cmd

.PHONY: torch-all
torch-all:
	@$(MAKE) -C $(HERE_PATH)/torch_include all

# 5) Aggregate: all
.PHONY: all
all: \
	torch-all \
	$(TEST_OUT)/test_iinuji_plots \
	$(TEST_OUT)/test_iinuji_logs \
	$(TEST_OUT)/test_iinuji_suit \
	$(TEST_OUT)/test_iinuji_mouse \
	$(TEST_OUT)/test_iinuji_toroid \
	$(TEST_OUT)/test_iinuji_tsi
	@$(LOG_SUCCESS)

# 6) Aggregate: run
.PHONY: run
run: \
	run-test_iinuji_plots \
	run-test_iinuji_logs \
	run-test_iinuji_suit \
	run-test_iinuji_mouse \
	run-test_iinuji_toroid \
	run-test_iinuji_cmd \
	run-test_iinuji_tsi

# 7) Clean
.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/test_iinuji_*
