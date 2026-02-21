// ./include/tsiemene/utils/circuits.h
// SPDX-License-Identifier: MIT
#pragma once

#include <cstddef>
#include <functional>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "tsiemene/utils/tsi.h"

namespace tsiemene {

struct Query { std::string_view text{}; };
[[nodiscard]] constexpr Query query(std::string_view t = {}) noexcept { return Query{t}; }

struct Endpoint {
  const Tsi* tsi{};
  DirectiveId directive{};
};
[[nodiscard]] constexpr Endpoint ep(const Tsi& t, DirectiveId d) noexcept {
  return Endpoint{&t, d};
}

struct Hop {
  Endpoint from{}; // Out directive
  Endpoint to{};   // In directive
  Query q{};       // opaque (not parsed here)
};

[[nodiscard]] constexpr Hop hop(Endpoint from, Endpoint to, Query q = {}) noexcept {
  return Hop{.from = from, .to = to, .q = q};
}

struct Circuit {
  const Hop* hops{};
  std::size_t hop_count{};
  std::string_view doc{};
};

template <std::size_t N>
[[nodiscard]] constexpr Circuit circuit(const Hop (&hops)[N], std::string_view doc = {}) noexcept {
  return Circuit{.hops = hops, .hop_count = N, .doc = doc};
}

[[nodiscard]] inline const DirectiveSpec* find_directive(const Tsi& t,
                                                         DirectiveId id,
                                                         DirectiveDir dir) noexcept {
  for (const DirectiveSpec& d : t.directives()) {
    if (d.id == id && d.dir == dir) return &d;
  }
  return nullptr;
}

struct CircuitIssue {
  std::string_view what{};
  std::size_t hop_index{};
};

[[nodiscard]] inline bool validate(const Circuit& c, CircuitIssue* issue = nullptr) noexcept {
  if (c.hop_count == 0) {
    if (issue) { issue->what = "empty circuit"; issue->hop_index = 0; }
    return false;
  }
  if (!c.hops) {
    if (issue) { issue->what = "null hops pointer"; issue->hop_index = 0; }
    return false;
  }

  const Tsi* start = c.hops[0].from.tsi;
  if (!start) {
    if (issue) { issue->what = "null tsi pointer"; issue->hop_index = 0; }
    return false;
  }

  std::unordered_map<const Tsi*, std::vector<const Tsi*>> adj;
  std::unordered_map<const Tsi*, std::size_t> in_degree;
  std::unordered_map<const Tsi*, std::size_t> out_degree;
  std::unordered_set<const Tsi*> nodes;

  // directive existence + metadata compatibility
  for (std::size_t i = 0; i < c.hop_count; ++i) {
    const Hop& h = c.hops[i];
    if (!h.from.tsi || !h.to.tsi) {
      if (issue) { issue->what = "null tsi pointer"; issue->hop_index = i; }
      return false;
    }

    const DirectiveSpec* a = find_directive(*h.from.tsi, h.from.directive, DirectiveDir::Out);
    const DirectiveSpec* b = find_directive(*h.to.tsi,   h.to.directive, DirectiveDir::In);
    if (!a || !b) {
      if (issue) { issue->what = "directive not found on tsi"; issue->hop_index = i; }
      return false;
    }

    DirectiveIssue pi{};
    if (!compatible(*a, *b, &pi)) {
      if (issue) { issue->what = pi.what; issue->hop_index = i; }
      return false;
    }

    nodes.insert(h.from.tsi);
    nodes.insert(h.to.tsi);
    adj[h.from.tsi].push_back(h.to.tsi);
    (void)adj[h.to.tsi];
    ++in_degree[h.to.tsi];
    (void)in_degree[h.from.tsi];
    ++out_degree[h.from.tsi];
    (void)out_degree[h.to.tsi];
  }

  if (nodes.find(start) == nodes.end()) {
    if (issue) { issue->what = "start tsi not part of graph"; issue->hop_index = 0; }
    return false;
  }

  std::vector<const Tsi*> roots;
  roots.reserve(nodes.size());
  for (const Tsi* n : nodes) {
    const auto in_it = in_degree.find(n);
    const std::size_t id = (in_it == in_degree.end()) ? 0 : in_it->second;
    if (id == 0) roots.push_back(n);
  }

  if (roots.empty()) {
    if (issue) { issue->what = "circuit has no root node"; issue->hop_index = 0; }
    return false;
  }
  if (roots.size() != 1) {
    if (issue) { issue->what = "circuit must have exactly one root node"; issue->hop_index = 0; }
    return false;
  }
  if (roots[0] != start) {
    if (issue) { issue->what = "first hop must start from circuit root"; issue->hop_index = 0; }
    return false;
  }

  bool start_has_input = false;
  for (const DirectiveSpec& d : start->directives()) {
    if (is_in(d.dir)) {
      start_has_input = true;
      break;
    }
  }
  if (!start_has_input) {
    if (issue) { issue->what = "start tsi has no input directives"; issue->hop_index = 0; }
    return false;
  }

  // DAG check on graph reachable from root.
  std::unordered_map<const Tsi*, int> color; // 0=white, 1=gray, 2=black
  bool cycle = false;
  std::unordered_set<const Tsi*> reachable;

  std::function<void(const Tsi*)> dfs = [&](const Tsi* u) {
    if (cycle || !u) return;
    color[u] = 1;
    reachable.insert(u);
    auto it = adj.find(u);
    if (it != adj.end()) {
      for (const Tsi* v : it->second) {
        const int cstate = color[v];
        if (cstate == 1) { cycle = true; return; }
        if (cstate == 0) dfs(v);
        if (cycle) return;
      }
    }
    color[u] = 2;
  };
  dfs(start);

  if (cycle) {
    if (issue) { issue->what = "cycle detected in circuit graph"; issue->hop_index = 0; }
    return false;
  }

  if (reachable.size() != nodes.size()) {
    for (std::size_t i = 0; i < c.hop_count; ++i) {
      const Hop& h = c.hops[i];
      if (!reachable.count(h.from.tsi) || !reachable.count(h.to.tsi)) {
        if (issue) { issue->what = "unreachable tsi from circuit root"; issue->hop_index = i; }
        return false;
      }
    }
    if (issue) { issue->what = "unreachable tsi from circuit root"; issue->hop_index = 0; }
    return false;
  }

  // Any terminal node in the graph must be a sink.
  for (const Tsi* n : nodes) {
    const auto od_it = out_degree.find(n);
    const std::size_t od = (od_it == out_degree.end()) ? 0 : od_it->second;
    if (od == 0 && !n->is_sink()) {
      if (issue) { issue->what = "terminal tsi must be tsi_sink"; issue->hop_index = 0; }
      return false;
    }
  }

  return true;
}

} // namespace tsiemene
