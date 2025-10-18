# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config

# -----------------------------------------------------------------------------
# Local Paths
# -----------------------------------------------------------------------------
HERE_PATH  := $(TESTS_PATH)/bench/iinuji/
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

# -----------------------------------------------------------------------------
# Build + run aliases
# -----------------------------------------------------------------------------
$(eval $(call TEST_ONEFILE,test_iinuji_plots,  test_iinuji_plots.cpp))
$(eval $(call TEST_ONEFILE,test_iinuji_logs,   test_iinuji_logs.cpp))
$(eval $(call TEST_ONEFILE,test_iinuji_suit, 	 test_iinuji_suit.cpp))
$(eval $(call TEST_ONEFILE,test_iinuji_mouse,  test_iinuji_mouse.cpp))
$(eval $(call TEST_ONEFILE,test_iinuji_toroid, test_iinuji_toroid.cpp))

# ---- Attach ncurses deps (headers + link flags) -----------------------------
# Headers for compile:
$(TEST_OUT)/test_iinuji_plots:  INCLUDES_EXTRA += $(NCURSES_INCLUDE_PATHS)
$(TEST_OUT)/test_iinuji_logs:   INCLUDES_EXTRA += $(NCURSES_INCLUDE_PATHS)
$(TEST_OUT)/test_iinuji_suit:   INCLUDES_EXTRA += $(NCURSES_INCLUDE_PATHS)
$(TEST_OUT)/test_iinuji_mouse:  INCLUDES_EXTRA += $(NCURSES_INCLUDE_PATHS)
$(TEST_OUT)/test_iinuji_toroid: INCLUDES_EXTRA += $(NCURSES_INCLUDE_PATHS)

# Link flags/libs:
# (TEST_DEFAULT_LDLIBS already adds $(libcommon_a) to keep the default)
$(TEST_OUT)/test_iinuji_plots:  LDLIBS_EXTRA   += $(NCURSES_LD_LAGS)
$(TEST_OUT)/test_iinuji_logs:   LDLIBS_EXTRA   += $(NCURSES_LD_LAGS)
$(TEST_OUT)/test_iinuji_suit:   LDLIBS_EXTRA   += $(NCURSES_LD_LAGS)
$(TEST_OUT)/test_iinuji_mouse:  LDLIBS_EXTRA   += $(NCURSES_LD_LAGS)
$(TEST_OUT)/test_iinuji_toroid: LDLIBS_EXTRA   += $(NCURSES_LD_LAGS)

# -----------------------------------------------------------------------------
# Aggregate build
# -----------------------------------------------------------------------------
.PHONY: all
all: \
	$(OUTPUT_PATH)/test_iinuji_plots \
	$(OUTPUT_PATH)/test_iinuji_logs \
	$(OUTPUT_PATH)/test_iinuji_suit \
	$(OUTPUT_PATH)/test_iinuji_mouse \
	$(OUTPUT_PATH)/test_iinuji_toroid
	@$(LOG_SUCCESS)

# -----------------------------------------------------------------------------
# Aggregate run
# -----------------------------------------------------------------------------
.PHONY: run
run: \
     run-test_iinuji_plots \
     run-test_iinuji_logs \
     run-test_iinuji_suit \
     run-test_iinuji_mouse \
     run-test_iinuji_toroid

# -----------------------------------------------------------------------------
# Clean
# -----------------------------------------------------------------------------
.PHONY: clean
clean:
	@rm -f $(OUTPUT_PATH)/test_iinuji_*
