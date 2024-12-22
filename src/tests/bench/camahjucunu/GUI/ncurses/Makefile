# -----------------------------------------------------------------
# Dependencies - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
ROOT_PATH := ../../../../..
include $(ROOT_PATH)/Makefile.config
# -----------------------------------------------------------------
# Local Makefile Variables - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
HERE_PATH := $(ROOT_PATH)/tests/bench/camahjucunu/GUI/ncurses
TEST_BUILD := $(ROOT_PATH)/tests/build
COMMON_NCURSES_COMPILE_CMD = \
	$(LOG_START) && $(LOCAL_CLEAN) && \
	$(GXX) $(GXX_FLAGS) \
	$(USUAL_INCLUDE_PATHS) $(NCURSES_INCLUDE_PATHS) \
	$(USUAL_LD_FLAGS) $< -o $@ $(NCURSES_LD_LAGS) && $(LOG_END)
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_iinuji_plots test_iinuji_plots
$(TEST_BUILD)/test_iinuji_plots: $(HERE_PATH)/test_iinuji_plots.cpp
	@$(COMMON_NCURSES_COMPILE_CMD)
	$(RUN_MODE) $(TEST_BUILD)/test_iinuji_plots
test_iinuji_plots: $(TEST_BUILD)/test_iinuji_plots
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_iinuji_logs test_iinuji_logs
$(TEST_BUILD)/test_iinuji_logs: $(HERE_PATH)/test_iinuji_logs.cpp
	@$(COMMON_NCURSES_COMPILE_CMD)
	$(RUN_MODE) $(TEST_BUILD)/test_iinuji_logs
test_iinuji_logs: $(TEST_BUILD)/test_iinuji_logs
# -----------------------------------------------------------------
.PHONY: $(TEST_BUILD)/test_iinuji_render test_iinuji_render
$(TEST_BUILD)/test_iinuji_render: $(HERE_PATH)/test_iinuji_render.cpp
	@$(COMMON_NCURSES_COMPILE_CMD)
	$(RUN_MODE) $(TEST_BUILD)/test_iinuji_render
test_iinuji_render: $(TEST_BUILD)/test_iinuji_render
# -----------------------------------------------------------------
# - - - General Target -- - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: all
all: $(TEST_BUILD)/test_iinuji_plots
	@$(LOG_SUCCESS)
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------