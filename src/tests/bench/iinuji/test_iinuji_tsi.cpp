// test_iinuji_tsi.cpp
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <ncursesw/ncurses.h>

#include "piaabo/dconfig.h"
#include "camahjucunu/BNF/implementations/tsiemene_board/tsiemene_board.h"
#include "camahjucunu/BNF/implementations/tsiemene_board/tsiemene_board_runtime.h"

#include "iinuji/iinuji_types.h"
#include "iinuji/iinuji_utils.h"
#include "iinuji/iinuji_render.h"
#include "iinuji/ncurses/iinuji_app_ncurses.h"

namespace {

using cuwacunu::camahjucunu::tsiemene_board_instruction_t;
using cuwacunu::camahjucunu::tsiemene_circuit_decl_t;
using cuwacunu::camahjucunu::tsiemene_resolved_hop_t;

struct BoardViewData {
  bool ok{false};
  std::string error{};
  std::string raw_instruction{};
  tsiemene_board_instruction_t board{};
  std::vector<std::vector<tsiemene_resolved_hop_t>> resolved_hops{};
};

static std::string short_type(std::string_view full) {
  const std::size_t p = full.rfind('.');
  if (p == std::string_view::npos) return std::string(full);
  return std::string(full.substr(p + 1));
}

static std::string trim_to_width(const std::string& s, int width) {
  if (width <= 0) return {};
  if ((int)s.size() <= width) return s;
  if (width <= 3) return s.substr(0, (std::size_t)width);
  return s.substr(0, (std::size_t)(width - 3)) + "...";
}

static void put_canvas_char(std::vector<std::string>& canvas, int x, int y, char ch) {
  if (y < 0 || y >= (int)canvas.size()) return;
  if (x < 0 || x >= (int)canvas[(std::size_t)y].size()) return;

  char& cell = canvas[(std::size_t)y][(std::size_t)x];
  if (cell == ' ' || cell == ch) {
    cell = ch;
    return;
  }

  const bool old_h = (cell == '-' || cell == '>' || cell == '<');
  const bool old_v = (cell == '|');
  const bool new_h = (ch == '-' || ch == '>' || ch == '<');
  const bool new_v = (ch == '|');
  if ((old_h && new_v) || (old_v && new_h) || cell == '+' || ch == '+') {
    cell = '+';
    return;
  }
  if (ch == '>' || ch == '<') {
    cell = ch;
  }
}

static void draw_hline(std::vector<std::string>& canvas, int x0, int x1, int y, char ch = '-') {
  if (x0 > x1) std::swap(x0, x1);
  for (int x = x0; x <= x1; ++x) put_canvas_char(canvas, x, y, ch);
}

static void draw_vline(std::vector<std::string>& canvas, int x, int y0, int y1, char ch = '|') {
  if (y0 > y1) std::swap(y0, y1);
  for (int y = y0; y <= y1; ++y) put_canvas_char(canvas, x, y, ch);
}

static void draw_text(std::vector<std::string>& canvas, int x, int y, const std::string& text) {
  if (y < 0 || y >= (int)canvas.size()) return;
  if (x >= (int)canvas[(std::size_t)y].size()) return;
  if (x < 0) return;
  std::string& row = canvas[(std::size_t)y];
  for (int i = 0; i < (int)text.size() && (x + i) < (int)row.size(); ++i) {
    row[(std::size_t)(x + i)] = text[(std::size_t)i];
  }
}

static void draw_box(std::vector<std::string>& canvas,
                     int x,
                     int y,
                     int w,
                     const std::string& line1,
                     const std::string& line2) {
  if (w < 4) return;
  draw_hline(canvas, x, x + w - 1, y, '-');
  draw_hline(canvas, x, x + w - 1, y + 3, '-');
  draw_vline(canvas, x, y, y + 3, '|');
  draw_vline(canvas, x + w - 1, y, y + 3, '|');
  put_canvas_char(canvas, x, y, '+');
  put_canvas_char(canvas, x + w - 1, y, '+');
  put_canvas_char(canvas, x, y + 3, '+');
  put_canvas_char(canvas, x + w - 1, y + 3, '+');

  draw_text(canvas, x + 1, y + 1, trim_to_width(line1, w - 2));
  draw_text(canvas, x + 1, y + 2, trim_to_width(line2, w - 2));
}

static std::string join_lines(const std::vector<std::string>& lines) {
  std::ostringstream oss;
  for (std::size_t i = 0; i < lines.size(); ++i) {
    oss << lines[i];
    if (i + 1 < lines.size()) oss << '\n';
  }
  return oss.str();
}

static std::string make_circuit_canvas(const tsiemene_circuit_decl_t& c,
                                       const std::vector<tsiemene_resolved_hop_t>& hops) {
  if (c.instances.empty()) return "(no instances)";

  const int kBoxW = 24;
  const int kBoxH = 4;
  const int kHGap = 7;
  const int kVGap = 2;
  const int kPadX = 2;
  const int kPadY = 1;

  std::unordered_map<std::string, std::size_t> alias_to_idx;
  alias_to_idx.reserve(c.instances.size());
  for (std::size_t i = 0; i < c.instances.size(); ++i) {
    alias_to_idx.emplace(c.instances[i].alias, i);
  }

  std::vector<std::vector<std::size_t>> adj(c.instances.size());
  std::vector<int> indeg(c.instances.size(), 0);
  for (const auto& h : hops) {
    const auto it_from = alias_to_idx.find(h.from.instance);
    const auto it_to = alias_to_idx.find(h.to.instance);
    if (it_from == alias_to_idx.end() || it_to == alias_to_idx.end()) continue;
    const std::size_t u = it_from->second;
    const std::size_t v = it_to->second;
    adj[u].push_back(v);
    ++indeg[v];
  }

  std::vector<int> indeg_work = indeg;
  std::deque<std::size_t> q;
  for (std::size_t i = 0; i < indeg_work.size(); ++i) {
    if (indeg_work[i] == 0) q.push_back(i);
  }
  std::vector<std::size_t> topo;
  topo.reserve(c.instances.size());
  while (!q.empty()) {
    const std::size_t u = q.front();
    q.pop_front();
    topo.push_back(u);
    for (const std::size_t v : adj[u]) {
      if (--indeg_work[v] == 0) q.push_back(v);
    }
  }
  if (topo.size() != c.instances.size()) {
    topo.clear();
    for (std::size_t i = 0; i < c.instances.size(); ++i) topo.push_back(i);
  }

  std::vector<int> layer(c.instances.size(), 0);
  for (const std::size_t u : topo) {
    for (const std::size_t v : adj[u]) {
      layer[v] = std::max(layer[v], layer[u] + 1);
    }
  }

  int max_layer = 0;
  for (const int l : layer) max_layer = std::max(max_layer, l);
  std::vector<std::vector<std::size_t>> by_layer((std::size_t)(max_layer + 1));
  for (std::size_t i = 0; i < c.instances.size(); ++i) {
    by_layer[(std::size_t)layer[i]].push_back(i);
  }

  int max_rows = 1;
  for (const auto& g : by_layer) max_rows = std::max(max_rows, (int)g.size());

  const int width = kPadX + (max_layer + 1) * (kBoxW + kHGap) + 2;
  const int height = kPadY + max_rows * (kBoxH + kVGap) + 2;
  std::vector<std::string> canvas((std::size_t)height, std::string((std::size_t)width, ' '));

  struct XY {
    int x{0};
    int y{0};
  };
  std::vector<XY> pos(c.instances.size());
  for (int l = 0; l <= max_layer; ++l) {
    for (int r = 0; r < (int)by_layer[(std::size_t)l].size(); ++r) {
      const std::size_t idx = by_layer[(std::size_t)l][(std::size_t)r];
      const int x = kPadX + l * (kBoxW + kHGap);
      const int y = kPadY + r * (kBoxH + kVGap);
      pos[idx] = XY{.x = x, .y = y};

      const bool is_root = (indeg[idx] == 0);
      const std::string alias = is_root ? ("*" + c.instances[idx].alias) : c.instances[idx].alias;
      const std::string tshort = short_type(c.instances[idx].tsi_type);
      draw_box(canvas, x, y, kBoxW, alias, tshort);
    }
  }

  for (const auto& h : hops) {
    const auto it_from = alias_to_idx.find(h.from.instance);
    const auto it_to = alias_to_idx.find(h.to.instance);
    if (it_from == alias_to_idx.end() || it_to == alias_to_idx.end()) continue;

    const XY a = pos[it_from->second];
    const XY b = pos[it_to->second];

    const int sx = a.x + kBoxW;
    const int sy = a.y + 1;
    const int tx = b.x - 1;
    const int ty = b.y + 1;

    int midx = sx + std::max(2, (tx - sx) / 2);
    if (midx > tx) midx = tx;

    draw_hline(canvas, sx, midx, sy, '-');
    draw_vline(canvas, midx, sy, ty, '|');
    draw_hline(canvas, midx, tx, ty, '-');
    put_canvas_char(canvas, tx, ty, '>');
  }

  return join_lines(canvas);
}

static std::string make_circuit_info(const tsiemene_circuit_decl_t& c,
                                     const std::vector<tsiemene_resolved_hop_t>& hops,
                                     std::size_t ci,
                                     std::size_t total) {
  std::ostringstream oss;
  oss << "Circuit " << (ci + 1) << "/" << total << "\n";
  oss << "name:   " << c.name << "\n";
  oss << "invoke: " << c.invoke_name << "(\"" << c.invoke_payload << "\")\n";
  oss << "symbol: " << cuwacunu::camahjucunu::circuit_invoke_symbol(c) << "\n";
  oss << "\nInstances (" << c.instances.size() << ")\n";
  for (std::size_t i = 0; i < c.instances.size(); ++i) {
    const auto& inst = c.instances[i];
    oss << "  [" << i << "] " << inst.alias << " = " << inst.tsi_type << "\n";
  }
  oss << "\nHops (" << hops.size() << ")\n";
  for (std::size_t i = 0; i < hops.size(); ++i) {
    const auto& h = hops[i];
    oss << "  [" << i << "] "
        << h.from.instance << h.from.directive << tsiemene::kind_token(h.from.kind)
        << " -> "
        << h.to.instance << h.to.directive << tsiemene::kind_token(h.to.kind)
        << "\n";
  }
  oss << "\nKeys\n";
  oss << "  q quit\n";
  oss << "  Left/Right or p/n switch circuit\n";
  oss << "  r reload board instruction\n";
  return oss.str();
}

static BoardViewData load_board_from_config() {
  BoardViewData out{};
  out.raw_instruction = cuwacunu::piaabo::dconfig::config_space_t::tsiemene_board_instruction();

  auto parser = cuwacunu::camahjucunu::BNF::tsiemeneBoard();
  out.board = parser.decode(out.raw_instruction);

  std::string error;
  if (!cuwacunu::camahjucunu::validate_board_instruction(out.board, &error)) {
    out.ok = false;
    out.error = error;
    return out;
  }

  out.resolved_hops.clear();
  out.resolved_hops.reserve(out.board.circuits.size());
  for (std::size_t i = 0; i < out.board.circuits.size(); ++i) {
    std::vector<tsiemene_resolved_hop_t> rh;
    std::string resolve_error;
    if (!cuwacunu::camahjucunu::resolve_hops(out.board.circuits[i], &rh, &resolve_error)) {
      out.ok = false;
      out.error = "circuit[" + std::to_string(i) + "] " + resolve_error;
      return out;
    }
    out.resolved_hops.push_back(std::move(rh));
  }

  out.ok = true;
  return out;
}

static void set_text_content(const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& box,
                             std::string text) {
  auto tb = std::dynamic_pointer_cast<cuwacunu::iinuji::textBox_data_t>(box->data);
  if (!tb) return;
  tb->content = std::move(text);
}

static std::string make_status(const BoardViewData& b, std::size_t selected_idx) {
  std::ostringstream oss;
  if (!b.ok) {
    oss << "invalid board instruction: " << b.error
        << " | press r reload | q quit";
    return oss.str();
  }
  oss << "board circuits=" << b.board.circuits.size();
  if (!b.board.circuits.empty()) {
    oss << " selected=" << (selected_idx + 1) << "/" << b.board.circuits.size();
  }
  oss << " | Left/Right p/n switch | r reload | q quit";
  return oss.str();
}

} // namespace

int main() try {
  const char* config_folder = "/cuwacunu/src/config/";
  cuwacunu::piaabo::dconfig::config_space_t::change_config_file(config_folder);
  cuwacunu::piaabo::dconfig::config_space_t::update_config();

  cuwacunu::iinuji::NcursesAppOpts app_opts{};
  app_opts.input_timeout_ms = 60;
  cuwacunu::iinuji::NcursesApp app(app_opts);

  if (has_colors()) {
    start_color();
    use_default_colors();
  }
  cuwacunu::iinuji::set_global_background("#101014");

  using namespace cuwacunu::iinuji;

  auto root = create_grid_container(
      "root",
      {len_spec::px(3), len_spec::px(2), len_spec::frac(1.0)},
      {len_spec::frac(1.0)},
      0,
      0,
      iinuji_layout_t{layout_mode_t::Normalized, 0, 0, 1, 1, true},
      iinuji_style_t{"#D8D8D8", "#101014", false, "#5E5E68"});

  auto title = create_text_box(
      "title",
      "tsiemene board visualizer",
      true,
      text_align_t::Left,
      iinuji_layout_t{},
      iinuji_style_t{"#EDEDED", "#202028", true, "#6C6C75", true, false, " iinuji.tsi "});
  place_in_grid(title, 0, 0);
  root->add_child(title);

  auto status = create_text_box(
      "status",
      "",
      true,
      text_align_t::Left,
      iinuji_layout_t{},
      iinuji_style_t{"#B8B8BF", "#101014", false, "#101014"});
  place_in_grid(status, 1, 0);
  root->add_child(status);

  auto body = create_grid_container(
      "body",
      {len_spec::frac(1.0)},
      {len_spec::frac(0.70), len_spec::frac(0.30)},
      1,
      1,
      iinuji_layout_t{},
      iinuji_style_t{"#D8D8D8", "#101014", false, "#5E5E68"});
  place_in_grid(body, 2, 0);
  root->add_child(body);

  auto canvas_box = create_text_box(
      "canvas",
      "",
      false,
      text_align_t::Left,
      iinuji_layout_t{},
      iinuji_style_t{"#D0D0D0", "#101014", true, "#5E5E68", false, false, " circuit map "});
  place_in_grid(canvas_box, 0, 0);
  body->add_child(canvas_box);

  auto info_box = create_text_box(
      "info",
      "",
      false,
      text_align_t::Left,
      iinuji_layout_t{},
      iinuji_style_t{"#C2C2C8", "#101014", true, "#5E5E68", false, false, " details "});
  place_in_grid(info_box, 0, 1);
  body->add_child(info_box);

  BoardViewData board_view = load_board_from_config();
  std::size_t selected = 0;

  auto refresh_content = [&]() {
    if (!board_view.ok || board_view.board.circuits.empty()) {
      selected = 0;
      set_text_content(
          title,
          "tsiemene board visualizer - invalid instruction");

      std::ostringstream coss;
      coss << "Board instruction is invalid.\n\n";
      if (!board_view.error.empty()) coss << "error: " << board_view.error << "\n\n";
      coss << "Raw instruction:\n" << board_view.raw_instruction << "\n";
      set_text_content(canvas_box, coss.str());
      set_text_content(
          info_box,
          "Fix src/config/instructions/tsiemene_board.instruction and press 'r' to reload.\n");
      set_text_content(status, make_status(board_view, selected));
      return;
    }

    if (selected >= board_view.board.circuits.size()) selected = 0;
    const auto& c = board_view.board.circuits[selected];
    const auto& hops = board_view.resolved_hops[selected];

    set_text_content(
        title,
        "tsiemene board visualizer - " + c.name);
    set_text_content(canvas_box, make_circuit_canvas(c, hops));
    set_text_content(info_box, make_circuit_info(c, hops, selected, board_view.board.circuits.size()));
    set_text_content(status, make_status(board_view, selected));
  };

  refresh_content();

  while (true) {
    int H = 0, W = 0;
    getmaxyx(stdscr, H, W);
    layout_tree(root, Rect{0, 0, W, H});

    clear();
    render_tree(root);
    refresh();

    const int ch = getch();
    if (ch == ERR || ch == KEY_RESIZE) continue;

    if (ch == 'q') break;
    if (ch == 'r') {
      cuwacunu::piaabo::dconfig::config_space_t::update_config();
      board_view = load_board_from_config();
      selected = 0;
      refresh_content();
      continue;
    }

    if (board_view.ok && !board_view.board.circuits.empty()) {
      if (ch == KEY_RIGHT || ch == 'n') {
        selected = (selected + 1) % board_view.board.circuits.size();
        refresh_content();
      } else if (ch == KEY_LEFT || ch == 'p') {
        selected = (selected + board_view.board.circuits.size() - 1) % board_view.board.circuits.size();
        refresh_content();
      }
    }
  }

  return 0;
} catch (const std::exception& e) {
  endwin();
  std::fprintf(stderr, "[test_iinuji_tsi] exception: %s\n", e.what());
  return 1;
}
