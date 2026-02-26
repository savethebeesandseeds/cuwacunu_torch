#pragma once

#include <algorithm>
#include <array>
#include <cstdlib>
#include <limits>
#include <queue>
#include <vector>

#include "iinuji/iinuji_cmd/views/board/view.canvas.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

struct grid_point_t {
  int x{0};
  int y{0};
};

inline bool operator==(const grid_point_t& a, const grid_point_t& b) {
  return a.x == b.x && a.y == b.y;
}

inline bool route_path_on_grid(
    const std::vector<std::vector<bool>>& blocked,
    const std::vector<std::vector<int>>& edge_heat,
    grid_point_t start,
    grid_point_t goal,
    std::vector<grid_point_t>* out_path) {
  if (!out_path) return false;
  out_path->clear();
  if (blocked.empty() || blocked.front().empty()) return false;
  const int h = static_cast<int>(blocked.size());
  const int w = static_cast<int>(blocked.front().size());
  auto in_bounds = [&](int x, int y) {
    return x >= 0 && y >= 0 && x < w && y < h;
  };
  if (!in_bounds(start.x, start.y) || !in_bounds(goal.x, goal.y)) return false;
  if (start == goal) {
    out_path->push_back(start);
    return true;
  }

  struct node_t {
    int f{0};
    int g{0};
    int x{0};
    int y{0};
  };
  auto cmp = [](const node_t& a, const node_t& b) {
    if (a.f != b.f) return a.f > b.f;
    return a.g > b.g;
  };
  std::priority_queue<node_t, std::vector<node_t>, decltype(cmp)> open(cmp);

  constexpr int kInf = std::numeric_limits<int>::max() / 8;
  std::vector<std::vector<int>> dist((std::size_t)h, std::vector<int>((std::size_t)w, kInf));
  std::vector<std::vector<grid_point_t>> prev(
      (std::size_t)h,
      std::vector<grid_point_t>((std::size_t)w, grid_point_t{.x = -1, .y = -1}));

  auto heuristic = [&](int x, int y) {
    return (std::abs(goal.x - x) + std::abs(goal.y - y)) * 10;
  };
  auto walkable = [&](int x, int y) {
    if (!in_bounds(x, y)) return false;
    if (x == start.x && y == start.y) return true;
    if (x == goal.x && y == goal.y) return true;
    return !blocked[(std::size_t)y][(std::size_t)x];
  };

  static constexpr std::array<int, 4> kDx = {1, 0, 0, -1};
  static constexpr std::array<int, 4> kDy = {0, -1, 1, 0};

  dist[(std::size_t)start.y][(std::size_t)start.x] = 0;
  open.push(node_t{.f = heuristic(start.x, start.y), .g = 0, .x = start.x, .y = start.y});

  while (!open.empty()) {
    const node_t cur = open.top();
    open.pop();
    if (cur.g != dist[(std::size_t)cur.y][(std::size_t)cur.x]) continue;
    if (cur.x == goal.x && cur.y == goal.y) break;

    for (int i = 0; i < 4; ++i) {
      const int nx = cur.x + kDx[(std::size_t)i];
      const int ny = cur.y + kDy[(std::size_t)i];
      if (!walkable(nx, ny)) continue;

      int step = 10;
      const int heat = edge_heat[(std::size_t)ny][(std::size_t)nx];
      if (heat > 0) step += 8 + heat * 6;
      if (nx < cur.x) step += 2;

      const int g2 = cur.g + step;
      if (g2 >= dist[(std::size_t)ny][(std::size_t)nx]) continue;

      dist[(std::size_t)ny][(std::size_t)nx] = g2;
      prev[(std::size_t)ny][(std::size_t)nx] = grid_point_t{.x = cur.x, .y = cur.y};
      open.push(node_t{.f = g2 + heuristic(nx, ny), .g = g2, .x = nx, .y = ny});
    }
  }

  if (dist[(std::size_t)goal.y][(std::size_t)goal.x] >= kInf) return false;

  std::vector<grid_point_t> rev;
  for (grid_point_t at = goal;;) {
    rev.push_back(at);
    if (at == start) break;
    const grid_point_t p = prev[(std::size_t)at.y][(std::size_t)at.x];
    if (p.x < 0 || p.y < 0) return false;
    at = p;
  }
  out_path->assign(rev.rbegin(), rev.rend());
  return true;
}

inline void draw_routed_path(
    std::vector<std::string>& canvas,
    std::vector<std::vector<circuit_draw_style_t>>& styles,
    std::vector<std::vector<int>>& edge_heat,
    const std::vector<grid_point_t>& path,
    circuit_draw_style_t edge_style) {
  if (path.empty()) return;
  const int h = static_cast<int>(canvas.size());
  const int w = h > 0 ? static_cast<int>(canvas.front().size()) : 0;
  auto mark_heat = [&](int x, int y) {
    if (x < 0 || y < 0 || x >= w || y >= h) return;
    ++edge_heat[(std::size_t)y][(std::size_t)x];
  };
  if (path.size() == 1) {
    put_canvas_char(canvas, styles, path.front().x, path.front().y, '-', edge_style);
    mark_heat(path.front().x, path.front().y);
    return;
  }

  const auto& first = path.front();
  const auto& second = path[1];
  put_canvas_char(
      canvas,
      styles,
      first.x,
      first.y,
      (first.x != second.x) ? '-' : '|',
      edge_style);
  mark_heat(first.x, first.y);

  for (std::size_t i = 1; i < path.size(); ++i) {
    const auto& a = path[i - 1];
    const auto& b = path[i];
    const char step_ch = (a.x != b.x) ? '-' : '|';
    put_canvas_char(canvas, styles, b.x, b.y, step_ch, edge_style);
    mark_heat(b.x, b.y);
  }

  for (std::size_t i = 1; i + 1 < path.size(); ++i) {
    const auto& p0 = path[i - 1];
    const auto& p1 = path[i];
    const auto& p2 = path[i + 1];
    const int d1x = p1.x - p0.x;
    const int d1y = p1.y - p0.y;
    const int d2x = p2.x - p1.x;
    const int d2y = p2.y - p1.y;
    if (d1x != d2x || d1y != d2y) {
      put_canvas_char(canvas, styles, p1.x, p1.y, '+', edge_style);
    }
  }

  const auto& tail = path.back();
  const auto& prev = path[path.size() - 2];
  char arrow = '>';
  if (prev.x > tail.x) arrow = '<';
  else if (prev.y < tail.y) arrow = 'v';
  else if (prev.y > tail.y) arrow = '^';
  put_canvas_char(canvas, styles, tail.x, tail.y, arrow, edge_style);
}

inline int port_offset_for_rank(int rank, int count) {
  if (count <= 1) return 1;
  if (count == 2) return (rank == 0) ? 0 : 3;
  if (count == 3) {
    static constexpr std::array<int, 3> kMap = {0, 2, 3};
    return kMap[(std::size_t)std::clamp(rank, 0, 2)];
  }
  if (count == 4) return std::clamp(rank, 0, 3);
  static constexpr std::array<int, 4> kWrap = {0, 3, 1, 2};
  return kWrap[(std::size_t)(rank % 4)];
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
