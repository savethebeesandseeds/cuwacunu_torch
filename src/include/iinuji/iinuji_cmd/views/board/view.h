#pragma once

#include <deque>
#include <unordered_map>

#include "iinuji/iinuji_cmd/views/common.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {
inline void put_canvas_char(std::vector<std::string>& canvas, int x, int y, char ch) {
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
  if (ch == '>' || ch == '<') cell = ch;
}

inline void draw_hline(std::vector<std::string>& canvas, int x0, int x1, int y, char ch = '-') {
  if (x0 > x1) std::swap(x0, x1);
  for (int x = x0; x <= x1; ++x) put_canvas_char(canvas, x, y, ch);
}

inline void draw_vline(std::vector<std::string>& canvas, int x, int y0, int y1, char ch = '|') {
  if (y0 > y1) std::swap(y0, y1);
  for (int y = y0; y <= y1; ++y) put_canvas_char(canvas, x, y, ch);
}

inline void draw_text(std::vector<std::string>& canvas, int x, int y, const std::string& text) {
  if (y < 0 || y >= (int)canvas.size()) return;
  if (x < 0 || x >= (int)canvas[(std::size_t)y].size()) return;
  std::string& row = canvas[(std::size_t)y];
  for (int i = 0; i < (int)text.size() && (x + i) < (int)row.size(); ++i) {
    row[(std::size_t)(x + i)] = text[(std::size_t)i];
  }
}

inline void draw_box(std::vector<std::string>& canvas,
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

inline std::string join_lines(const std::vector<std::string>& lines) {
  std::ostringstream oss;
  for (std::size_t i = 0; i < lines.size(); ++i) {
    oss << lines[i];
    if (i + 1 < lines.size()) oss << '\n';
  }
  return oss.str();
}

inline std::string make_circuit_canvas(const tsiemene_circuit_decl_t& c,
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

inline std::string make_circuit_info(const tsiemene_circuit_decl_t& c,
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
  return oss.str();
}

inline std::string make_board_left(const CmdState& st) {
  if (!st.board.ok) {
    std::ostringstream oss;
    oss << "Board instruction invalid.\n\n";
    oss << "error: " << st.board.error << "\n\n";
    oss << "raw instruction:\n" << st.board.raw_instruction << "\n";
    return oss.str();
  }
  if (st.board.board.circuits.empty()) {
    return "Board has no circuits.";
  }
  const auto& c = st.board.board.circuits[st.board.selected_circuit];
  const auto& hops = st.board.resolved_hops[st.board.selected_circuit];
  return make_circuit_canvas(c, hops);
}

inline std::string make_board_right(const CmdState& st) {
  if (!st.board.ok) {
    return "Fix src/config/instructions/tsiemene_board.instruction\nthen run command: reload";
  }
  if (st.board.board.circuits.empty()) {
    return "No circuits.";
  }
  const auto& c = st.board.board.circuits[st.board.selected_circuit];
  const auto& hops = st.board.resolved_hops[st.board.selected_circuit];
  return make_circuit_info(c, hops, st.board.selected_circuit, st.board.board.circuits.size());
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
