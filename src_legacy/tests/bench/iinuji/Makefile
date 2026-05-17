# =============================================================================
# tests/bench/iinuji/Makefile — flattened iinuji bench tests
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
TORCH_LINK = $(libtorch_a) $(libcurl_a) $(libssl_a) $(libcommon_a) $(LDLIBS_torch) $(LDLIBS_curl) $(LDLIBS_ssl)

$(eval $(call TEST_ONEFILE, test_iinuji_plots,                test_iinuji_plots.cpp))
$(eval $(call TEST_ONEFILE, test_iinuji_logs,                 test_iinuji_logs.cpp))
$(eval $(call TEST_ONEFILE, test_iinuji_suit,                 test_iinuji_suit.cpp))
$(eval $(call TEST_ONEFILE, test_iinuji_mouse,                test_iinuji_mouse.cpp))
$(eval $(call TEST_ONEFILE, test_iinuji_toroid,               test_iinuji_toroid.cpp))
$(eval $(call TEST_ONEFILE, test_iinuji_tsi,                  test_iinuji_tsi.cpp))
$(eval $(call TEST_ONEFILE, test_iinuji_data_viz,             test_iinuji_data_viz.cpp,             $(TORCH_LINK)))
$(eval $(call TEST_ONEFILE, test_iinuji_timeseries,           test_iinuji_timeseries.cpp,           $(TORCH_LINK)))
$(eval $(call TEST_ONEFILE, test_iinuji_cmd,                  test_iinuji_cmd.cpp,                  $(TORCH_LINK)))
$(eval $(call TEST_ONEFILE, test_iinuji_bootstrap,            test_iinuji_bootstrap.cpp))
$(eval $(call TEST_ONEFILE, test_iinuji_cmd_landing,          test_iinuji_cmd_landing.cpp,          $(TORCH_LINK)))
$(eval $(call TEST_ONEFILE, test_iinuji_cmd_terminal,         test_iinuji_cmd_terminal.cpp,         $(TORCH_LINK)))
$(eval $(call TEST_ONEFILE, test_iinuji_image_primitive,      test_iinuji_image_primitive.cpp))
$(eval $(call TEST_ONEFILE, test_iinuji_image_demo,           test_iinuji_image_demo.cpp))
$(eval $(call TEST_ONEFILE, test_iinuji_animation_primitive,  test_iinuji_animation_primitive.cpp))
$(eval $(call TEST_ONEFILE, test_iinuji_animation_demo,       test_iinuji_animation_demo.cpp))
$(eval $(call TEST_ONEFILE, test_iinuji_braille_colors,       test_iinuji_braille_colors.cpp))
$(eval $(call TEST_ONEFILE, test_iinuji_art_text_primitive,   test_iinuji_art_text_primitive.cpp))
$(eval $(call TEST_ONEFILE, test_iinuji_art_text_demo,        test_iinuji_art_text_demo.cpp))

# 4) Target-specific includes/link flags
$(TEST_OUT)/test_iinuji_%: INCLUDES_EXTRA += $(NCURSES_INCLUDE_PATHS)
$(TEST_OUT)/test_iinuji_%: LDLIBS_EXTRA   += $(LDLIBS_ncurses)

$(TEST_OUT)/test_iinuji_data_viz: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/test_iinuji_timeseries: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/test_iinuji_cmd: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/test_iinuji_cmd_landing: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/test_iinuji_cmd_terminal: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/test_iinuji_image_demo: LDLIBS_EXTRA += -L/lib/x86_64-linux-gnu -L/usr/lib/x86_64-linux-gnu -l:libz.so.1
$(TEST_OUT)/test_iinuji_animation_primitive: LDLIBS_EXTRA += -L/lib/x86_64-linux-gnu -L/usr/lib/x86_64-linux-gnu -l:libz.so.1
$(TEST_OUT)/test_iinuji_animation_demo: LDLIBS_EXTRA += -L/lib/x86_64-linux-gnu -L/usr/lib/x86_64-linux-gnu -l:libz.so.1

# 5) Aggregate: all
.PHONY: all
all: \
	$(TEST_OUT)/test_iinuji_plots \
	$(TEST_OUT)/test_iinuji_logs \
	$(TEST_OUT)/test_iinuji_suit \
	$(TEST_OUT)/test_iinuji_mouse \
	$(TEST_OUT)/test_iinuji_toroid \
	$(TEST_OUT)/test_iinuji_tsi \
	$(TEST_OUT)/test_iinuji_data_viz \
	$(TEST_OUT)/test_iinuji_timeseries \
	$(TEST_OUT)/test_iinuji_cmd \
	$(TEST_OUT)/test_iinuji_bootstrap \
	$(TEST_OUT)/test_iinuji_cmd_landing \
	$(TEST_OUT)/test_iinuji_cmd_terminal \
	$(TEST_OUT)/test_iinuji_image_primitive \
	$(TEST_OUT)/test_iinuji_image_demo \
	$(TEST_OUT)/test_iinuji_animation_primitive \
	$(TEST_OUT)/test_iinuji_animation_demo \
	$(TEST_OUT)/test_iinuji_braille_colors \
	$(TEST_OUT)/test_iinuji_art_text_primitive \
	$(TEST_OUT)/test_iinuji_art_text_demo
	@$(LOG_SUCCESS)

# 6) Aggregate: run
.PHONY: run
run: \
	run-test_iinuji_plots \
	run-test_iinuji_logs \
	run-test_iinuji_suit \
	run-test_iinuji_mouse \
	run-test_iinuji_toroid \
	run-test_iinuji_tsi \
	run-test_iinuji_data_viz \
	run-test_iinuji_timeseries \
	run-test_iinuji_cmd \
	run-test_iinuji_cmd_landing \
	run-test_iinuji_cmd_terminal \
	run-test_iinuji_image_primitive \
	run-test_iinuji_image_demo \
	run-test_iinuji_animation_primitive \
	run-test_iinuji_animation_demo \
	run-test_iinuji_braille_colors \
	run-test_iinuji_art_text_primitive \
	run-test_iinuji_art_text_demo

# 7) Clean
.PHONY: clean
clean:
	$(Q)$(CLEAN_RM_CMD) $(TEST_OUT)/test_iinuji_*
