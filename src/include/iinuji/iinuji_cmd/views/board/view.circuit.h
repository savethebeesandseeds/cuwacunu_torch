#pragma once

#include <algorithm>
#include <cstdlib>
#include <deque>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "iinuji/iinuji_cmd/views/board/view.routing.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline std::string make_circuit_canvas(const tsiemene_circuit_decl_t& c,
                                       const std::vector<tsiemene_resolved_hop_t>& hops) {
  if (c.instances.empty()) return "(no instances)";

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

  std::vector<std::string> alias_labels(c.instances.size());
  std::vector<std::string> type_labels(c.instances.size());
  std::vector<circuit_draw_style_t> node_styles(c.instances.size(), circuit_draw_style_t::NodeWikimyei);
  int max_label_len = 0;
  for (std::size_t i = 0; i < c.instances.size(); ++i) {
    const bool is_root = (indeg[i] == 0);
    alias_labels[i] = (is_root ? "*" : "") + c.instances[i].alias;
    type_labels[i] = compact_tsi_type_label(c.instances[i].tsi_type);
    node_styles[i] = node_style_from_tsi_type(c.instances[i].tsi_type);
    max_label_len = std::max(max_label_len, (int)alias_labels[i].size());
    max_label_len = std::max(max_label_len, (int)type_labels[i].size());
  }

  const int density_hint = std::max(max_rows, max_layer + 1);
  const int kBoxH = 4;
  int kBoxW = 22;
  if (density_hint >= 8) kBoxW = 16;
  else if (density_hint >= 6) kBoxW = 18;
  else if (density_hint >= 4) kBoxW = 20;
  kBoxW = std::max(kBoxW, std::min(26, max_label_len + 2));
  const int kHGap = (density_hint >= 8) ? 4 : (density_hint >= 6) ? 5 : (density_hint >= 4) ? 6 : 8;
  const int kVGap = (max_rows >= 6) ? 2 : 3;
  const int kPadX = 1;
  const int kPadY = 1;

  const int content_h = max_rows * kBoxH + std::max(0, max_rows - 1) * kVGap;
  const int width = kPadX * 2 + (max_layer + 1) * kBoxW + std::max(0, max_layer) * kHGap + 1;
  const int height = kPadY * 2 + content_h + 1;
  std::vector<std::string> canvas((std::size_t)height, std::string((std::size_t)width, ' '));
  std::vector<std::vector<circuit_draw_style_t>> styles(
      (std::size_t)height,
      std::vector<circuit_draw_style_t>((std::size_t)width, circuit_draw_style_t::Default));

  struct XY {
    int x{0};
    int y{0};
  };
  std::vector<XY> pos(c.instances.size());

  for (int l = 0; l <= max_layer; ++l) {
    const int layer_count = (int)by_layer[(std::size_t)l].size();
    const int layer_h = layer_count * kBoxH + std::max(0, layer_count - 1) * kVGap;
    const int y0 = kPadY + std::max(0, (content_h - layer_h) / 2);
    for (int r = 0; r < layer_count; ++r) {
      const std::size_t idx = by_layer[(std::size_t)l][(std::size_t)r];
      const int x = kPadX + l * (kBoxW + kHGap);
      const int y = y0 + r * (kBoxH + kVGap);
      pos[idx] = XY{.x = x, .y = y};

      const bool is_root = (indeg[idx] == 0);
      draw_box(canvas,
               styles,
               x,
               y,
               kBoxW,
               alias_labels[idx],
               type_labels[idx],
               node_styles[idx],
               is_root ? circuit_draw_style_t::NodeRoot : circuit_draw_style_t::NodeAlias,
               circuit_draw_style_t::NodeType);
    }
  }

  std::vector<std::vector<bool>> blocked(
      (std::size_t)height,
      std::vector<bool>((std::size_t)width, false));
  std::vector<std::vector<int>> edge_heat(
      (std::size_t)height,
      std::vector<int>((std::size_t)width, 0));

  for (std::size_t i = 0; i < c.instances.size(); ++i) {
    const XY p = pos[i];
    for (int yy = p.y; yy <= p.y + kBoxH - 1; ++yy) {
      if (yy < 0 || yy >= height) continue;
      for (int xx = p.x; xx <= p.x + kBoxW - 1; ++xx) {
        if (xx < 0 || xx >= width) continue;
        blocked[(std::size_t)yy][(std::size_t)xx] = true;
      }
    }
  }

  std::vector<int> hop_from_idx(hops.size(), -1);
  std::vector<int> hop_to_idx(hops.size(), -1);
  std::vector<std::vector<std::size_t>> out_hops(c.instances.size());
  std::vector<std::vector<std::size_t>> in_hops(c.instances.size());

  for (std::size_t hi = 0; hi < hops.size(); ++hi) {
    const auto& h = hops[hi];
    const auto it_from = alias_to_idx.find(h.from.instance);
    const auto it_to = alias_to_idx.find(h.to.instance);
    if (it_from == alias_to_idx.end() || it_to == alias_to_idx.end()) continue;
    hop_from_idx[hi] = static_cast<int>(it_from->second);
    hop_to_idx[hi] = static_cast<int>(it_to->second);
    out_hops[it_from->second].push_back(hi);
    in_hops[it_to->second].push_back(hi);
  }

  std::vector<int> hop_out_offset(hops.size(), 1);
  std::vector<int> hop_in_offset(hops.size(), 1);

  for (std::size_t ni = 0; ni < c.instances.size(); ++ni) {
    auto& outv = out_hops[ni];
    std::sort(outv.begin(), outv.end(), [&](std::size_t a, std::size_t b) {
      const int ta = (hop_to_idx[a] >= 0) ? pos[(std::size_t)hop_to_idx[a]].y : 0;
      const int tb = (hop_to_idx[b] >= 0) ? pos[(std::size_t)hop_to_idx[b]].y : 0;
      if (ta != tb) return ta < tb;
      return hops[a].from.directive < hops[b].from.directive;
    });
    for (int rank = 0; rank < (int)outv.size(); ++rank) {
      hop_out_offset[outv[(std::size_t)rank]] = port_offset_for_rank(rank, (int)outv.size());
    }

    auto& inv = in_hops[ni];
    std::sort(inv.begin(), inv.end(), [&](std::size_t a, std::size_t b) {
      const int sa = (hop_from_idx[a] >= 0) ? pos[(std::size_t)hop_from_idx[a]].y : 0;
      const int sb = (hop_from_idx[b] >= 0) ? pos[(std::size_t)hop_from_idx[b]].y : 0;
      if (sa != sb) return sa < sb;
      return hops[a].to.directive < hops[b].to.directive;
    });
    for (int rank = 0; rank < (int)inv.size(); ++rank) {
      hop_in_offset[inv[(std::size_t)rank]] = port_offset_for_rank(rank, (int)inv.size());
    }
  }

  std::vector<std::size_t> hop_route_order{};
  hop_route_order.reserve(hops.size());
  for (std::size_t hi = 0; hi < hops.size(); ++hi) {
    if (hop_from_idx[hi] >= 0 && hop_to_idx[hi] >= 0) hop_route_order.push_back(hi);
  }
  std::sort(hop_route_order.begin(), hop_route_order.end(), [&](std::size_t a, std::size_t b) {
    const XY af = pos[(std::size_t)hop_from_idx[a]];
    const XY at = pos[(std::size_t)hop_to_idx[a]];
    const XY bf = pos[(std::size_t)hop_from_idx[b]];
    const XY bt = pos[(std::size_t)hop_to_idx[b]];
    const int sa = std::abs((at.x - af.x)) + std::abs((at.y - af.y));
    const int sb = std::abs((bt.x - bf.x)) + std::abs((bt.y - bf.y));
    if (sa != sb) return sa > sb;
    return a < b;
  });

  for (const std::size_t hi : hop_route_order) {
    const auto& h = hops[hi];
    const auto it_from = alias_to_idx.find(h.from.instance);
    const auto it_to = alias_to_idx.find(h.to.instance);
    if (it_from == alias_to_idx.end() || it_to == alias_to_idx.end()) continue;

    const std::size_t from_idx = it_from->second;
    const std::size_t to_idx = it_to->second;
    const XY a = pos[from_idx];
    const XY b = pos[to_idx];
    const circuit_draw_style_t edge_style = edge_style_from_directive(h.from.directive);

    const int sx = a.x + kBoxW;
    const int sy = a.y + hop_out_offset[hi];
    const int tx = b.x - 1;
    const int ty = b.y + hop_in_offset[hi];

    std::vector<grid_point_t> path;
    const bool routed = route_path_on_grid(
        blocked,
        edge_heat,
        grid_point_t{.x = sx, .y = sy},
        grid_point_t{.x = tx, .y = ty},
        &path);

    if (routed && !path.empty()) {
      draw_routed_path(canvas, styles, edge_heat, path, edge_style);
    } else {
      put_canvas_char(canvas, styles, sx, sy, 'x', edge_style);
      if (sx + 1 < width) put_canvas_char(canvas, styles, sx + 1, sy, '>', edge_style);
    }
  }

  return join_lines_ansi(canvas, styles) + make_edge_legend_text(hops);
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
