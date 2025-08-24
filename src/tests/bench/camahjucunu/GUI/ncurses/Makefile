# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------
ROOT_PATH := ../../../../..
include $(ROOT_PATH)/Makefile.config

# -----------------------------------------------------------------------------
# Local Paths
# -----------------------------------------------------------------------------
HERE_PATH  := $(TESTS_PATH)/bench/camahjucunu/GUI/ncurses
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

# -----------------------------------------------------------------------------
# Build + run aliases
# -----------------------------------------------------------------------------
$(eval $(call TEST_ONEFILE,test_iinuji_plots,  test_iinuji_plots.cpp))
$(eval $(call TEST_ONEFILE,test_iinuji_logs,   test_iinuji_logs.cpp))
$(eval $(call TEST_ONEFILE,test_iinuji_render, test_iinuji_render.cpp))

# ---- Attach ncurses deps (headers + link flags) -----------------------------
# Headers for compile:
$(TEST_OUT)/test_iinuji_plots:  INCLUDES_EXTRA += $(NCURSES_INCLUDE_PATHS)
$(TEST_OUT)/test_iinuji_logs:   INCLUDES_EXTRA += $(NCURSES_INCLUDE_PATHS)
$(TEST_OUT)/test_iinuji_render: INCLUDES_EXTRA += $(NCURSES_INCLUDE_PATHS)

# Link flags/libs:
# (TEST_DEFAULT_LDLIBS already adds $(libcommon_a) if you keep the default)
$(TEST_OUT)/test_iinuji_plots:  LDLIBS_EXTRA   += $(NCURSES_LD_LAGS)
$(TEST_OUT)/test_iinuji_logs:   LDLIBS_EXTRA   += $(NCURSES_LD_LAGS)
$(TEST_OUT)/test_iinuji_render: LDLIBS_EXTRA   += $(NCURSES_LD_LAGS)

# -----------------------------------------------------------------------------
# Aggregate build
# -----------------------------------------------------------------------------
.PHONY: all
all: \
	$(OUTPUT_PATH)/test_iinuji_plots \
	$(OUTPUT_PATH)/test_iinuji_logs  \
	$(OUTPUT_PATH)/test_iinuji_render
	@$(LOG_SUCCESS)

# -----------------------------------------------------------------------------
# Aggregate run
# -----------------------------------------------------------------------------
.PHONY: run
run: \
     run-test_iinuji_plots \
     run-test_iinuji_logs  \
     run-test_iinuji_render

# -----------------------------------------------------------------------------
# Clean
# -----------------------------------------------------------------------------
.PHONY: clean
clean:
	@rm -f $(OUTPUT_PATH)/test_iinuji_*
