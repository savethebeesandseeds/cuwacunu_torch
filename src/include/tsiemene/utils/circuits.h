// ./include/tsiemene/circuits.h
// SPDX-License-Identifier: MIT
#pragma once

#include <cstddef>
#include <string_view>
#include <vector>

#include "tsiemene/utils/tsi.h"

namespace tsiemene {

struct Query { std::string_view text{}; };
[[nodiscard]] constexpr Query query(std::string_view t = {}) noexcept { return Query{t}; }

struct Endpoint {
  const Tsi* tsi{};
  PortId port{};
};
[[nodiscard]] constexpr Endpoint ep(const Tsi& t, PortId p) noexcept { return Endpoint{&t, p}; }

struct Hop {
  Endpoint from{}; // Out
  Endpoint to{};   // In
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

[[nodiscard]] inline const Port* find_port(const Tsi& t, PortId id) noexcept {
  for (const Port& p : t.ports()) if (p.id == id) return &p;
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

  // chain continuity: hop[i].to.tsi == hop[i+1].from.tsi
  for (std::size_t i = 0; i + 1 < c.hop_count; ++i) {
    if (c.hops[i].to.tsi != c.hops[i + 1].from.tsi) {
      if (issue) { issue->what = "chain broken (to.tsi != next.from.tsi)"; issue->hop_index = i; }
      return false;
    }
  }

  // sink constraint: last hop's destination tsi must be sink
  const Endpoint& last = c.hops[c.hop_count - 1].to;
  if (!last.tsi || !last.tsi->is_sink()) {
    if (issue) { issue->what = "last tsi must be tsi_sink"; issue->hop_index = c.hop_count - 1; }
    return false;
  }

  // acyclic in node-sequence: n0=hop0.from.tsi, n1=hop0.to.tsi, n2=hop1.to.tsi, ...
  // (allowing the shared middle tsi via continuity; duplicates only if you "return" later)
  {
    std::vector<const Tsi*> nodes;
    nodes.reserve(c.hop_count + 1);
    const Tsi* start = c.hops[0].from.tsi;
    if (!start) {
      if (issue) { issue->what = "null tsi pointer"; issue->hop_index = 0; }
      return false;
    }
    nodes.push_back(start);

    for (std::size_t i = 0; i < c.hop_count; ++i) {
      const Tsi* nxt = c.hops[i].to.tsi;
      if (!nxt) {
        if (issue) { issue->what = "null tsi pointer"; issue->hop_index = i; }
        return false;
      }
      for (const Tsi* s : nodes) {
        if (s == nxt) {
          if (issue) { issue->what = "cycle: tsi repeats in chain"; issue->hop_index = i; }
          return false;
        }
      }
      nodes.push_back(nxt);
    }
  }

  // port existence + metadata compatibility
  for (std::size_t i = 0; i < c.hop_count; ++i) {
    const Hop& h = c.hops[i];
    if (!h.from.tsi || !h.to.tsi) {
      if (issue) { issue->what = "null tsi pointer"; issue->hop_index = i; }
      return false;
    }

    const Port* a = find_port(*h.from.tsi, h.from.port);
    const Port* b = find_port(*h.to.tsi,   h.to.port);
    if (!a || !b) {
      if (issue) { issue->what = "port not found on tsi"; issue->hop_index = i; }
      return false;
    }

    PortIssue pi{};
    if (!compatible(*a, *b, &pi)) {
      if (issue) { issue->what = pi.what; issue->hop_index = i; }
      return false;
    }
  }

  return true;
}

} // namespace tsiemene
