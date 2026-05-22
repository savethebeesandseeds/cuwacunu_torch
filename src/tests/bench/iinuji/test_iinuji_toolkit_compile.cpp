#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "iinuji/iinuji_render.h"
#include "iinuji/ncurses/iinuji_app_ncurses.h"
#include "iinuji/ncurses/iinuji_keys_ncurses.h"
#include "iinuji/primitives/animation.h"
#include "iinuji/primitives/art_text.h"
#include "iinuji/primitives/editor.h"
#include "iinuji/primitives/image.h"
#include "iinuji/primitives/plot.h"

namespace {

bool require(bool condition, const std::string &message) {
  if (!condition) {
    std::cerr << "[FAIL] " << message << '\n';
    return false;
  }
  return true;
}

class CountingRenderer final : public cuwacunu::iinuji::IRend {
public:
  int text_calls{0};
  int glyph_calls{0};
  int fill_calls{0};
  int filled_cells{0};

  void size(int &h, int &w) override {
    h = 24;
    w = 80;
  }

  void clear() override {}
  void flush() override {}

  void putText(int, int, const std::string &s, int, short, bool,
               bool) override {
    ++text_calls;
    filled_cells += static_cast<int>(s.size());
  }

  void putGlyph(int, int, wchar_t, short) override { ++glyph_calls; }

  void fillRect(int, int, int h, int w, short) override {
    ++fill_calls;
    filled_cells += std::max(0, h) * std::max(0, w);
  }
};

} // namespace

int main() {
  using namespace cuwacunu::iinuji;

  bool ok = true;

  auto root = create_grid_container(
      "root", {len_spec::px(3), len_spec::frac(1.0)},
      {len_spec::frac(0.45), len_spec::frac(0.55)}, 0, 1, iinuji_layout_t{},
      iinuji_style_t{"#d8d8d8", "#101014"});

  text_box_opts_t title_opts{};
  title_opts.content = "iinuji toolkit";
  title_opts.wrap = false;
  title_opts.style = iinuji_style_t{"#ededed", "#202028", true, "#6c6c75"};
  auto title = create_text_box("title", std::move(title_opts));
  place_in_grid(title, 0, 0, 1, 2);
  root->add_child(title);

  auto panel =
      create_panel("panel", iinuji_layout_t{},
                   iinuji_style_t{"#d0d0d0", "#101014", true, "#5e5e68"});
  place_in_grid(panel, 1, 0);
  root->add_child(panel);

  auto body = create_text_box("body", "alpha\nbeta", true);
  panel_body_object(panel)->add_child(body);

  plotbox_opts_t plot_opts{};
  plot_opts.x_label = "x";
  plot_opts.y_label = "y";
  std::vector<std::vector<std::pair<double, double>>> series{
      {{0.0, 1.0}, {1.0, 2.0}, {2.0, 1.5}}};
  std::vector<plot_series_cfg_t> series_cfg(1);
  series_cfg[0].color_fg = "#ffc857";
  auto plot = create_plot_box("plot", series, series_cfg, plot_opts);
  place_in_grid(plot, 1, 1);
  root->add_child(plot);
  image_box_opts_t image_box_opts{};
  auto image_box = create_image_box("image", std::move(image_box_opts));
  ok = require(image_box->primitive_role == primitive_role_t::ImageBox,
               "image box should declare primitive role") &&
       ok;

  layout_tree(root, Rect{0, 0, 100, 30});
  ok = require(root->screen.w == 100 && root->screen.h == 30,
               "root layout should use requested bounds") &&
       ok;
  ok = require(title->screen.w > 0 && title->screen.h > 0,
               "title should receive a grid cell") &&
       ok;
  ok = require(panel->children.size() == 1, "panel should own one body") && ok;
  ok = require(plot->screen.x > panel->screen.x,
               "plot should be placed to the right of panel") &&
       ok;
  ok = require(root->primitive_role == primitive_role_t::Container,
               "grid container should declare primitive role") &&
       ok;
  ok = require(panel->primitive_role == primitive_role_t::Panel,
               "panel should declare primitive role") &&
       ok;

  CountingRenderer renderer{};
  IRend *old_renderer = set_renderer(&renderer);
  render_tree(root);
  set_renderer(old_renderer);
  ok = require(renderer.fill_calls > 0, "render should fill widget surfaces") &&
       ok;
  ok = require(renderer.text_calls + renderer.glyph_calls > 0,
               "render should emit visible text or glyphs") &&
       ok;
  ok = require(renderer.filled_cells > 0,
               "render should draw into the in-memory renderer") &&
       ok;

  editorBox_data_t editor{"memory.dsl"};
  primitives::editor_set_text(editor, "first\nsecond");
  auto move_result =
      primitives::editor_handle_key(editor, translate_ncurses_key(KEY_END));
  ok = require(move_result.handled && !move_result.content_changed,
               "translated ncurses key should move the editor cursor") &&
       ok;
  auto insert_result = primitives::editor_handle_key(editor, '!');
  ok = require(insert_result.handled && insert_result.content_changed,
               "printable key should insert through editor key handling") &&
       ok;
  ok = require(primitives::editor_text(editor) == "first!\nsecond",
               "editor insert should update the first line") &&
       ok;
  auto undo_result = primitives::editor_handle_key(editor, key_ctrl_z);
  ok = require(undo_result.handled && undo_result.undo,
               "toolkit key should drive editor undo") &&
       ok;
  ok = require(primitives::editor_text(editor) == "first\nsecond",
               "editor undo should restore previous text") &&
       ok;
  ok = require(translate_ncurses_key(KEY_BACKSPACE) == key_backspace,
               "ncurses backspace should translate to toolkit key") &&
       ok;
  ok = require(translate_ncurses_key('x') == 'x',
               "plain input should pass through key translation") &&
       ok;
  ok = require(standard_key_action(key_ctrl_z) == key_action_t::Undo,
               "standard key bindings should map undo") &&
       ok;

  auto focus_root = create_grid_container("focus-root", {len_spec::frac(1.0)},
                                          {len_spec::frac(1.0)}, 0, 0);
  auto input = create_input_line("input", "query");
  auto editor_object = create_editor_box("editor", "memory.dsl");
  focus_root->add_child(input);
  focus_root->add_child(editor_object);
  auto state = initialize_iinuji_state(focus_root, false);
  ok = require(state->focused == input && input->focused,
               "state should focus the first focusable primitive") &&
       ok;
  ok = require(focus_next_object(*state) && state->focused == editor_object &&
                   !input->focused && editor_object->focused,
               "focus traversal should move between focusable primitives") &&
       ok;
  ok = require(
           input->focus_mode == focus_mode_t::Input &&
               editor_object->primitive_role == primitive_role_t::EditorBox,
           "primitive factories should standardize focus and role metadata") &&
       ok;
  ok = require(handle_text_box_key(input, 'x').content_changed,
               "text box key handler should edit input content") &&
       ok;
  auto input_data = std::dynamic_pointer_cast<text_box_data_t>(input->data);
  ok = require(input_data && input_data->content == "queryx",
               "text box input handler should update model text") &&
       ok;
  buffer_box_opts_t buffer_opts{};
  buffer_opts.capacity = 8;
  buffer_opts.focus_mode = focus_mode_t::Focusable;
  auto buffer = create_buffer_box("buffer", buffer_opts);
  auto buffer_data = std::dynamic_pointer_cast<buffer_box_data_t>(buffer->data);
  if (buffer_data) {
    buffer_data->push_line("one");
    buffer_data->push_line("two");
  }
  ok = require(buffer_data != nullptr, "buffer factory should create data") &&
       ok;
  ok = require(handle_buffer_box_key(buffer, key_up).viewport_changed,
               "buffer key handler should scroll viewport") &&
       ok;

  rgba_image_t image{};
  rgba_animation_t animation{};
  ok = require(!image.valid(), "default image should be invalid") && ok;
  ok = require(!animation.valid(), "default animation should be invalid") && ok;

  NcursesAppOpts opts{};
  opts.use_dev_tty = false;
  opts.fallback_initscr = false;
  opts.input_timeout_ms = 0;
  ok = require(
           !opts.use_dev_tty && !opts.fallback_initscr &&
               opts.input_timeout_ms == 0,
           "ncurses options should be configurable without initialization") &&
       ok;

  return ok ? 0 : 1;
}
