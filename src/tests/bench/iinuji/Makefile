ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config

HERE_PATH := $(TESTS_PATH)/bench/iinuji
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

TEST_DEFAULT_LDLIBS :=
IINUJI_TEST_LDLIBS := $(LDLIBS_ncurses) -L/usr/lib/x86_64-linux-gnu -l:libz.so.1

$(eval $(call TEST_ONEFILE, test_iinuji_toolkit_compile, test_iinuji_toolkit_compile.cpp, $(IINUJI_TEST_LDLIBS)))
$(eval $(call TEST_ONEFILE, test_iinuji_image_box_render, test_iinuji_image_box_render.cpp, $(IINUJI_TEST_LDLIBS)))
$(eval $(call TEST_ONEFILE, test_iinuji_cmd_nav_rail, test_iinuji_cmd_nav_rail.cpp, $(IINUJI_TEST_LDLIBS)))
$(eval $(call TEST_ONEFILE, test_iinuji_html_server, test_iinuji_html_server.cpp,))

$(TEST_OUT)/test_iinuji_toolkit_compile: INCLUDES_EXTRA += $(NCURSES_INCLUDE_PATHS)
$(TEST_OUT)/test_iinuji_image_box_render: INCLUDES_EXTRA += $(NCURSES_INCLUDE_PATHS)
$(TEST_OUT)/test_iinuji_cmd_nav_rail: INCLUDES_EXTRA += $(NCURSES_INCLUDE_PATHS)

IINUJI_HEADER_SMOKE := \
	iinuji/iinuji_ansi.h \
	iinuji/iinuji_color.h \
	iinuji/iinuji_keys.h \
	iinuji/iinuji_primitives.h \
	iinuji/iinuji_render.h \
	iinuji/iinuji_types.h \
	iinuji/iinuji_utils.h \
	iinuji/html/iinuji_http_server.h \
	iinuji/html/synthetic_chart_api.h \
	iinuji/ncurses/iinuji_app_ncurses.h \
	iinuji/ncurses/iinuji_keys_ncurses.h \
	iinuji/ncurses/iinuji_render_ncurses.h \
	iinuji/primitives/animation.h \
	iinuji/primitives/art_text.h \
	iinuji/primitives/buffer_box.h \
	iinuji/primitives/editor.h \
	iinuji/primitives/grid_container.h \
	iinuji/primitives/image.h \
	iinuji/primitives/panel.h \
	iinuji/primitives/plot.h \
	iinuji/primitives/plot_box.h \
	iinuji/primitives/primitive.h \
	iinuji/primitives/text_box.h \
	iinuji/render/border_render.h \
	iinuji/render/buffer_box_render.h \
	iinuji/render/editor_box_render.h \
	iinuji/render/image_box_render.h \
	iinuji/render/layout_core.h \
	iinuji/render/panel_render.h \
	iinuji/render/plot_box_render.h \
	iinuji/render/renderer.h \
	iinuji/render/text_box_render.h \
	iinuji/render/tree_render.h

IINUJI_PURE_HEADER_SMOKE := \
	iinuji/iinuji_keys.h \
	iinuji/iinuji_utils.h \
	iinuji/html/iinuji_http_server.h \
	iinuji/html/synthetic_chart_api.h \
	iinuji/primitives/buffer_box.h \
	iinuji/primitives/editor.h \
	iinuji/primitives/grid_container.h \
	iinuji/primitives/panel.h \
	iinuji/primitives/plot_box.h \
	iinuji/primitives/primitive.h \
	iinuji/primitives/text_box.h

.PHONY: header-smoke
header-smoke:
	@mkdir -p $(OBJTMP)
	@for header in $(IINUJI_HEADER_SMOKE); do \
		obj="$(OBJTMP)/iinuji_header_$$(printf '%s' "$$header" | tr '/.' '__').o"; \
		$(CXX_COMPILE) $(CXXFLAGS) $(USUAL_INCLUDE_PATHS) $(NCURSES_INCLUDE_PATHS) \
			-x c++ -include "$(SRC_ROOT)/include/$$header" -c /dev/null -o "$$obj"; \
	done

.PHONY: pure-header-smoke
pure-header-smoke:
	@mkdir -p $(OBJTMP)
	@for header in $(IINUJI_PURE_HEADER_SMOKE); do \
		obj="$(OBJTMP)/iinuji_pure_header_$$(printf '%s' "$$header" | tr '/.' '__').o"; \
		$(CXX_COMPILE) $(CXXFLAGS) $(USUAL_INCLUDE_PATHS) \
			-x c++ -include "$(SRC_ROOT)/include/$$header" -c /dev/null -o "$$obj"; \
	done

.PHONY: all
all: $(TEST_OUT)/test_iinuji_toolkit_compile $(TEST_OUT)/test_iinuji_image_box_render $(TEST_OUT)/test_iinuji_cmd_nav_rail $(TEST_OUT)/test_iinuji_html_server header-smoke pure-header-smoke
	@$(LOG_SUCCESS)

.PHONY: run
run: run-test_iinuji_toolkit_compile

.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/test_iinuji_toolkit_compile $(TEST_OUT)/test_iinuji_image_box_render $(TEST_OUT)/test_iinuji_cmd_nav_rail $(TEST_OUT)/test_iinuji_html_server
