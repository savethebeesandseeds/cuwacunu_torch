/* iinuji_types.h */
#pragma once
#include <algorithm>
#include <deque>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace cuwacunu {
namespace iinuji {

/* -------------------- Rect -------------------- */
struct Rect {
  int x{0}, y{0}, w{0}, h{0};
};

/* -------------------- Length specs (px or fraction) -------------------- */
enum class unit_t { Px, Frac };
struct len_spec {
  unit_t u{unit_t::Frac};
  double v{1.0};
  static len_spec px(int p) { return len_spec{unit_t::Px, (double)p}; }
  static len_spec frac(double f) { return len_spec{unit_t::Frac, f}; }
};

/* -------------------- Grid spec for containers -------------------- */
struct grid_spec_t {
  std::vector<len_spec> rows;
  std::vector<len_spec> cols;
  int gap_row{0}, gap_col{0};
  int pad_left{0}, pad_right{0}, pad_top{0}, pad_bottom{0};
};

/* -------------------- Layout modes -------------------- */
enum class layout_mode_t { Absolute, Normalized, Dock, GridCell };
enum class dock_t { None, Top, Bottom, Left, Right, Fill };

struct iinuji_layout_t {
  layout_mode_t mode{layout_mode_t::Normalized};

  // Absolute / Normalized
  double x{0}, y{0}, width{1}, height{1};
  bool normalized{true};

  // Dock
  dock_t dock{dock_t::None};
  len_spec dock_size =
      len_spec::frac(0.2); // height for Top/Bottom; width for Left/Right

  // GridCell
  int grid_row{0}, grid_col{0}, grid_row_span{1}, grid_col_span{1};

  // Padding for content area
  int pad_left{0}, pad_right{0}, pad_top{0}, pad_bottom{0};
};

/* -------------------- Style -------------------- */
struct iinuji_style_t {
  std::string label_color{"white"};
  std::string background_color{"black"};
  bool border{false};
  std::string border_color{"gray"};
  bool bold{false};
  bool inverse{false};
  std::string title{};
};

/* -------------------- Data types -------------------- */
struct iinuji_data_t {
  virtual ~iinuji_data_t() = default;
};

struct panel_data_t : public iinuji_data_t {};

enum class text_align_t { Left, Center, Right };
enum class text_line_emphasis_t {
  None = 0,
  Accent,
  Success,
  Fatal,
  Error,
  MutedError,
  Warning,
  MutedWarning,
  Info,
  Debug,
};

struct styled_text_segment_t {
  std::string text{};
  text_line_emphasis_t emphasis{text_line_emphasis_t::None};
};

struct styled_text_line_t {
  std::string text{};
  text_line_emphasis_t emphasis{text_line_emphasis_t::None};
  std::vector<styled_text_segment_t> segments{};
  std::string background_color{};
};

inline std::string styled_text_line_text(const styled_text_line_t &line) {
  if (line.segments.empty())
    return line.text;
  std::size_t total = 0;
  for (const auto &segment : line.segments)
    total += segment.text.size();
  std::string out{};
  out.reserve(total);
  for (const auto &segment : line.segments)
    out += segment.text;
  return out;
}

inline styled_text_line_t make_segmented_styled_text_line(
    std::vector<styled_text_segment_t> segments,
    text_line_emphasis_t fallback_emphasis = text_line_emphasis_t::None) {
  styled_text_line_t line{};
  line.segments = std::move(segments);
  line.text = styled_text_line_text(line);
  line.emphasis = fallback_emphasis;
  return line;
}

struct textBox_data_t : public iinuji_data_t {
  std::string content;
  std::vector<styled_text_line_t> styled_lines{};
  bool wrap{true};
  text_align_t align{text_align_t::Left};
  // Tracks the last selected row auto-revealed by the UI so manual scrolling
  // is preserved until the selection itself changes.
  int tracked_selected_row{-1};
  int tracked_selected_row_end{-1};
  // Viewport scroll offsets used by the renderer for non-input text boxes.
  int scroll_y{0};
  int scroll_x{0};
  textBox_data_t(std::string s, bool w = true,
                 text_align_t a = text_align_t::Left)
      : content(std::move(s)), wrap(w), align(a) {}

  void scroll_by(int dy, int dx = 0) {
    scroll_y = std::max(0, scroll_y + dy);
    scroll_x = std::max(0, scroll_x + dx);
  }

  void clear_styled_lines() { styled_lines.clear(); }
};

/* -------------------- Text editor box -------------------- */
struct editorBox_data_t : public iinuji_data_t {
  struct history_frame_t {
    std::vector<std::string> lines{};
    int cursor_line{0};
    int cursor_col{0};
    int preferred_col{-1};
    int top_line{0};
    int left_col{0};
    bool dirty{false};
  };

  enum class history_merge_kind_t {
    None = 0,
    InsertChar,
    BackspaceChar,
    DeleteChar,
  };

  using line_colorizer_t = std::function<void(
      const editorBox_data_t &editor, int line_index, const std::string &line,
      std::vector<short> &out_colors, short base_pair,
      const std::string &background_color)>;

  std::string path;
  std::vector<std::string> lines;
  bool dirty{false};
  bool read_only{false};
  bool close_armed{false}; // Ctrl+Q twice to discard if dirty
  bool close_armed_via_escape{false};

  // Cursor + viewport (0-based)
  int cursor_line{0};
  int cursor_col{0};
  int preferred_col{-1}; // for vertical motion
  int top_line{0};
  int left_col{0};

  // Render caches (best-effort; runtime may use)
  int last_body_h{0};
  int last_lineno_w{0};
  int last_text_w{0};

  int tab_width{2};
  bool show_header{true};
  bool show_footer{true};
  bool show_line_numbers{true};
  bool show_tabs{true};
  bool highlight_current_line{true};
  bool highlight_matching_delimiter{true};
  std::string status;
  line_colorizer_t line_colorizer{};
  bool history_enabled{true};
  std::size_t history_limit{128};
  std::vector<history_frame_t> undo_stack{};
  std::vector<history_frame_t> redo_stack{};
  history_merge_kind_t history_merge_kind{history_merge_kind_t::None};
  int history_merge_cursor_line{-1};
  int history_merge_cursor_col{-1};
  int history_merge_line_count{0};

  explicit editorBox_data_t(std::string p = "") : path(std::move(p)) {
    lines.emplace_back(); // always at least one line
  }

  void ensure_nonempty() {
    if (lines.empty())
      lines.emplace_back();
    cursor_line = std::clamp(cursor_line, 0, (int)lines.size() - 1);
    cursor_col =
        std::clamp(cursor_col, 0, (int)lines[(size_t)cursor_line].size());
    if (top_line < 0)
      top_line = 0;
    if (left_col < 0)
      left_col = 0;
  }
};

/* -------------------- Buffer box -------------------- */
enum class buffer_dir_t { UpDown, DownUp };

/* A single rendered line in a buffer box.
   - text  : the main payload line
   - label : optional event label (e.g. "INFO", "ERROR")
   - color : optional per-line override for text color
             if empty => use the widget/style default
   (iinuji_object_t::style.label_color)

   This is meant to support EVENT metadata like:
     EVENT _update
       __name stdout
       __label INFO
       __color #ffffff
       __form  str:.sys.stdout
     ENDEVENT
*/
struct buffer_line_t {
  std::string text;
  std::string label; // optional
  std::string color; // optional; if empty => use widget default text color
};

struct bufferBox_data_t : public iinuji_data_t {
  std::deque<buffer_line_t> lines;

  std::size_t capacity{1000}; // max number of lines
  buffer_dir_t dir{buffer_dir_t::UpDown};

  // scroll == 0 means "tail" / newest visible.
  // scroll > 0 means user scrolled away to older content.
  int scroll{0};

  // If user is at tail (scroll==0), follow new lines.
  // If user scrolls up, stop following until scroll returns to 0.
  bool follow_tail{true};

  // Best-effort width hint (in columns) from last render. Used to keep
  // the view stable while wrapped when new lines arrive.
  int wrap_width_last{0};

  bufferBox_data_t(std::size_t cap = 1000,
                   buffer_dir_t d = buffer_dir_t::UpDown)
      : capacity(std::max<std::size_t>(1, cap)), dir(d) {}

  // Backwards-compatible API (old callers): pushes a plain text line with no
  // metadata.
  void push_line(std::string s) {
    push_line(std::move(s), /*label*/ "", /*color*/ "");
  }

  // New API: push a line with optional label and color override.
  // NOTE: color is a string token (e.g. "#ff0000"). The renderer decides how to
  // map it.
  void push_line(std::string s, std::string label, std::string color) {
    // normalize line endings lightly
    if (!s.empty() && s.back() == '\r')
      s.pop_back();

    // If the user is NOT at the tail (scroll>0), we want to *freeze* the view.
    // Since scroll is "distance from tail", every appended line increases that
    // distance by 1. This prevents the visible window from shifting while
    // reading old logs.
    const bool was_at_tail = (scroll == 0);

    auto estimate_added_rows = [&](std::size_t text_len,
                                   std::size_t prefix_len) -> int {
      const int W = wrap_width_last;
      if (W <= 0)
        return 1;
      int avail = W - (int)prefix_len;
      if (avail <= 0)
        avail = 1;
      if (text_len == 0)
        return 1;
      return 1 + (int)((text_len - 1) / (std::size_t)avail);
    };

    buffer_line_t L;
    L.text = std::move(s);
    L.label = std::move(label);
    L.color = std::move(color);

    lines.push_back(std::move(L));

    while (lines.size() > capacity) {
      lines.pop_front();
    }

    if (!was_at_tail) {
      // user is reading history → keep the same content visible
      follow_tail = false;
      std::size_t prefix_len = 0;
      if (!L.label.empty())
        prefix_len = L.label.size() + 3; // "[" + label + "] "
      scroll += estimate_added_rows(L.text.size(), prefix_len);
    } else {
      // user is at tail → follow newest
      follow_tail = true;
      scroll = 0;
    }

    // Render clamps based on viewport + wrap width.
    if (scroll < 0)
      scroll = 0;
  }

  void clear() {
    lines.clear();
    scroll = 0;
    follow_tail = true;
  }

  // scroll_by(+k) => move toward older content
  // scroll_by(-k) => move toward newer content (tail)
  void scroll_by(int delta) {
    scroll = std::max(0, scroll + delta);
    if (scroll > 0)
      follow_tail = false;
    if (scroll == 0)
      follow_tail = true;
  }

  void jump_tail() {
    scroll = 0;
    follow_tail = true;
  }
};

/* Plot config (decoupled from plotter header) */
enum class plot_mode_t { Line, Scatter, Stairs, Stem };

/* Envelope source for overlay */
enum class envelope_source_t { OriginalSamples, SegmentPath };

struct plot_series_cfg_t {
  std::string color_fg; // e.g. "#FFC857" or "yellow"
  short color_pair{-1}; // prebuilt pair; leave -1 to use color_fg

  plot_mode_t mode{plot_mode_t::Line};
  bool scatter{false};
  int scatter_every{1};

  bool fill_vertical_if_same_x{true}; // helps “needle” spikes
  double stem_y{std::numeric_limits<double>::quiet_NaN()}; // baseline for Stem

  // Envelope overlay controls (applies when mode==Line)
  bool envelope_enabled{false};
  envelope_source_t envelope_source{envelope_source_t::OriginalSamples};
  int envelope_min_count{2};
  int envelope_min_height{2};
  bool envelope_draw_base{true};
};

struct plotbox_opts_t {
  bool draw_axes{true}, draw_grid{true}, baseline0{true};
  int y_ticks{5}, x_ticks{6};
  double x_min{std::numeric_limits<double>::quiet_NaN()};
  double x_max{std::numeric_limits<double>::quiet_NaN()};
  double y_min{std::numeric_limits<double>::quiet_NaN()};
  double y_max{std::numeric_limits<double>::quiet_NaN()};
  bool hard_clip{true};
  bool x_log{false}, y_log{false};
  double x_log_eps{1e-12}, y_log_eps{1e-12};
  std::string x_label{}, y_label{};
  int margin_left{8}, margin_right{2}, margin_top{1}, margin_bot{2};
};

struct plotBox_data_t : public iinuji_data_t {
  std::vector<std::vector<std::pair<double, double>>> series;
  std::vector<plot_series_cfg_t> series_cfg;
  plotbox_opts_t opts;
};

/* -------------------- Events -------------------- */
enum class event_type {
  Key,
  MouseDown,
  MouseUp,
  MouseMove,
  Wheel,
  Resize,
  Timer,
  Custom
};

struct event_t {
  event_type type{event_type::Custom};
  int key{0};
  int x{0}, y{0};
  int button{0};
  int delta{0};
  int width{0}, height{0};
  std::string name;
  std::string payload;
};

struct iinuji_object_t;
struct iinuji_state_t;
using event_handler_t = std::function<void(
    iinuji_state_t &, std::shared_ptr<iinuji_object_t> &, const event_t &)>;

/* -------------------- Object -------------------- */
struct iinuji_object_t : public std::enable_shared_from_this<iinuji_object_t> {
  long id_num{0};
  std::string id;
  bool visible{true};
  int z_index{0};
  // Focus / tab navigation (runtime)
  bool focusable{false}; // figures set this true
  bool focused{false};   // exactly one per screen (by convention)

  iinuji_layout_t layout{};
  iinuji_style_t style{};
  std::shared_ptr<iinuji_data_t> data{};

  // Layout runtime
  Rect screen{};

  // Container extras
  std::shared_ptr<grid_spec_t> grid;

  // Tree
  std::weak_ptr<iinuji_object_t> parent;
  std::vector<std::shared_ptr<iinuji_object_t>> children;

  // Event listeners
  std::unordered_map<event_type, std::vector<event_handler_t>> listeners;

  void add_child(const std::shared_ptr<iinuji_object_t> &c) {
    c->parent = shared_from_this();
    children.push_back(c);
  }
  void add_children(const std::vector<std::shared_ptr<iinuji_object_t>> &v) {
    for (auto &c : v)
      add_child(c);
  }

  void on(event_type t, const event_handler_t &fn) {
    listeners[t].push_back(fn);
  }
  void off(event_type t) { listeners.erase(t); }
  void toggle_visible() { visible = !visible; }
};

/* -------------------- State w/ ID registry -------------------- */
struct iinuji_state_t {
  std::shared_ptr<iinuji_object_t> root;
  std::shared_ptr<iinuji_object_t> focused;
  bool running{true};
  bool in_ncurses_mode{true};
  int last_key{0};

  std::unordered_map<std::string, std::weak_ptr<iinuji_object_t>> id_index;

  std::shared_ptr<iinuji_object_t> by_id(const std::string &name) const {
    auto it = id_index.find(name);
    if (it == id_index.end())
      return nullptr;
    return it->second.lock();
  }
  void register_id(const std::string &name,
                   const std::shared_ptr<iinuji_object_t> &o) {
    if (!name.empty())
      id_index[name] = o, o->id = name;
  }
};

/* -------------------- Fabrics -------------------- */
inline std::shared_ptr<iinuji_state_t>
initialize_iinuji_state(const std::shared_ptr<iinuji_object_t> &root,
                        bool in_ncurses_mode = true) {
  auto st = std::make_shared<iinuji_state_t>();
  st->root = root;
  st->focused = root;
  st->running = true;
  st->in_ncurses_mode = in_ncurses_mode;
  return st;
}

inline std::shared_ptr<iinuji_object_t>
create_object(const std::string &id = "", bool visible = true,
              const iinuji_layout_t &layout = {},
              const iinuji_style_t &style = {}) {
  static long counter = 0;
  auto o = std::make_shared<iinuji_object_t>();
  o->id_num = counter++;
  o->visible = visible;
  o->layout = layout;
  o->style = style;
  o->id = id;
  return o;
}

inline std::shared_ptr<iinuji_object_t>
create_buffer_box(const std::string &id, std::size_t capacity, buffer_dir_t dir,
                  const iinuji_layout_t &layout = {},
                  const iinuji_style_t &style = {}) {
  auto o = create_object(id, true, layout, style);
  o->data = std::make_shared<bufferBox_data_t>(capacity, dir);
  return o;
}

inline std::shared_ptr<iinuji_object_t>
create_panel(const std::string &id, const iinuji_layout_t &layout = {},
             const iinuji_style_t &style = {}) {
  auto o = create_object(id, true, layout, style);
  o->data = std::make_shared<panel_data_t>();

  auto body = create_object(id.empty() ? std::string{} : id + ".body", true,
                            iinuji_layout_t{},
                            iinuji_style_t{style.label_color,
                                           style.background_color,
                                           false,
                                           style.border_color,
                                           style.bold,
                                           style.inverse,
                                           {}});
  body->layout.mode = layout_mode_t::Dock;
  body->layout.dock = dock_t::Fill;
  o->add_child(body);
  return o;
}

inline std::shared_ptr<iinuji_object_t>
create_text_box(const std::string &id, std::string content, bool wrap = true,
                text_align_t align = text_align_t::Left,
                const iinuji_layout_t &layout = {},
                const iinuji_style_t &style = {}) {
  auto o = create_object(id, true, layout, style);
  o->data = std::make_shared<textBox_data_t>(std::move(content), wrap, align);
  return o;
}

inline std::shared_ptr<iinuji_object_t> create_plot_box(
    const std::string &id,
    const std::vector<std::vector<std::pair<double, double>>> &series,
    const std::vector<plot_series_cfg_t> &cfg, const plotbox_opts_t &opts,
    const iinuji_layout_t &layout = {}, const iinuji_style_t &style = {}) {
  auto o = create_object(id, true, layout, style);
  auto pb = std::make_shared<plotBox_data_t>();
  pb->series = series;
  pb->series_cfg = cfg;
  pb->opts = opts;
  o->data = pb;
  return o;
}

/* Grid container helper */
inline std::shared_ptr<iinuji_object_t>
create_grid_container(const std::string &id, const std::vector<len_spec> &rows,
                      const std::vector<len_spec> &cols, int gap_row = 0,
                      int gap_col = 0, const iinuji_layout_t &layout = {},
                      const iinuji_style_t &style = {}) {
  auto o = create_object(id, true, layout, style);
  o->grid = std::make_shared<grid_spec_t>();
  o->grid->rows = rows;
  o->grid->cols = cols;
  o->grid->gap_row = gap_row;
  o->grid->gap_col = gap_col;
  return o;
}

inline bool is_panel_object(const std::shared_ptr<iinuji_object_t> &object) {
  return object != nullptr &&
         std::dynamic_pointer_cast<panel_data_t>(object->data) != nullptr;
}

inline std::shared_ptr<iinuji_object_t>
panel_body_object(const std::shared_ptr<iinuji_object_t> &object) {
  if (!is_panel_object(object) || object->children.empty())
    return object;
  return object->children.front();
}

/* Place child in a grid cell */
inline void place_in_grid(std::shared_ptr<iinuji_object_t> child, int r, int c,
                          int rs = 1, int cs = 1) {
  child->layout.mode = layout_mode_t::GridCell;
  child->layout.grid_row = r;
  child->layout.grid_col = c;
  child->layout.grid_row_span = rs;
  child->layout.grid_col_span = cs;
}

} // namespace iinuji
} // namespace cuwacunu
