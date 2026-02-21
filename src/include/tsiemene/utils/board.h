// ./include/tsiemene/utils/board.h
// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "tsiemene/utils/runtime.h"

namespace tsiemene {

// High-order container: a board owns circuits.
// Each circuit owns its node instances and the hop graph between them.
struct BoardCircuit {
  std::string name{};
  std::string invoke_name{};
  std::string invoke_payload{};

  std::vector<std::unique_ptr<Tsi>> nodes{};
  std::vector<Hop> hops{};

  // Default execution seed for this circuit.
  Wave wave0{};
  Ingress ingress0{};

  template <class NodeT, class... Args>
  NodeT& emplace_node(Args&&... args) {
    auto p = std::make_unique<NodeT>(std::forward<Args>(args)...);
    NodeT& ref = *p;
    nodes.push_back(std::move(p));
    return ref;
  }

  [[nodiscard]] Circuit view() const noexcept {
    return Circuit{
      .hops = hops.data(),
      .hop_count = hops.size(),
      .doc = name
    };
  }
};

struct Board {
  std::vector<BoardCircuit> circuits{};
};

[[nodiscard]] inline DirectiveId pick_start_directive(const Circuit& c) noexcept {
  if (!c.hops || c.hop_count == 0 || !c.hops[0].from.tsi) {
    return directive_id::Payload;
  }

  const Tsi& t = *c.hops[0].from.tsi;
  for (const auto& d : t.directives()) {
    if (d.dir == DirectiveDir::In && d.kind.kind == PayloadKind::String) return d.id;
  }
  for (const auto& d : t.directives()) {
    if (d.dir == DirectiveDir::In) return d.id;
  }
  return directive_id::Payload;
}

[[nodiscard]] inline bool validate_circuit(const BoardCircuit& c, CircuitIssue* issue = nullptr) noexcept {
  return validate(c.view(), issue);
}

struct BoardIssue {
  std::string_view what{};
  std::size_t circuit_index{};
  CircuitIssue circuit_issue{};
};

[[nodiscard]] inline bool validate_board(const Board& b, BoardIssue* issue = nullptr) noexcept {
  if (b.circuits.empty()) {
    if (issue) {
      issue->what = "empty board";
      issue->circuit_index = 0;
      issue->circuit_issue = CircuitIssue{.what = "empty board", .hop_index = 0};
    }
    return false;
  }

  for (std::size_t i = 0; i < b.circuits.size(); ++i) {
    const BoardCircuit& c = b.circuits[i];
    auto fail = [&](std::string_view what, std::size_t hop_index) noexcept {
      if (issue) {
        issue->what = what;
        issue->circuit_index = i;
        issue->circuit_issue = CircuitIssue{.what = what, .hop_index = hop_index};
      }
      return false;
    };

    if (c.name.empty()) return fail("circuit name is empty", 0);
    if (c.invoke_name.empty()) return fail("circuit invoke_name is empty", 0);
    if (c.invoke_payload.empty()) return fail("circuit invoke_payload is empty", 0);
    if (c.nodes.empty()) return fail("circuit has no nodes", 0);

    std::unordered_set<const Tsi*> owned_nodes;
    std::unordered_set<TsiId> node_ids;
    owned_nodes.reserve(c.nodes.size());
    node_ids.reserve(c.nodes.size());
    for (const auto& n : c.nodes) {
      if (!n) return fail("null node in circuit nodes", 0);
      if (!owned_nodes.insert(n.get()).second) return fail("duplicated node pointer in circuit nodes", 0);
      if (!node_ids.insert(n->id()).second) return fail("duplicated tsi id in circuit nodes", 0);
    }

    std::unordered_set<const Tsi*> wired_nodes;
    wired_nodes.reserve(c.nodes.size());
    for (std::size_t hi = 0; hi < c.hops.size(); ++hi) {
      const Hop& h = c.hops[hi];
      if (!owned_nodes.count(h.from.tsi) || !owned_nodes.count(h.to.tsi)) {
        return fail("hop endpoint is not owned by circuit nodes", hi);
      }
      wired_nodes.insert(h.from.tsi);
      wired_nodes.insert(h.to.tsi);
    }
    if (wired_nodes.size() != owned_nodes.size()) {
      return fail("orphan node not referenced by any hop", 0);
    }

    CircuitIssue ci{};
    if (!validate_circuit(c, &ci)) {
      if (issue) {
        issue->what = "invalid circuit";
        issue->circuit_index = i;
        issue->circuit_issue = ci;
      }
      return false;
    }

    const Circuit cv = c.view();
    if (!cv.hops || cv.hop_count == 0 || !cv.hops[0].from.tsi) {
      return fail("circuit has no start tsi", 0);
    }

    if (c.ingress0.directive.empty()) return fail("circuit ingress0.directive is empty", 0);

    const DirectiveSpec* start_in =
        find_directive(*cv.hops[0].from.tsi, c.ingress0.directive, DirectiveDir::In);
    if (!start_in) return fail("circuit ingress0 directive not found on root tsi", 0);

    if (start_in->kind.kind != c.ingress0.signal.kind) {
      return fail("circuit ingress0 kind mismatch with root tsi input", 0);
    }
  }
  return true;
}

inline std::uint64_t run_circuit(const BoardCircuit& c, TsiContext& ctx) {
  return run_wave(c.view(), c.wave0, c.ingress0, ctx);
}

inline std::uint64_t run_board(const Board& b, TsiContext& ctx) {
  std::uint64_t total = 0;
  for (const auto& c : b.circuits) total += run_circuit(c, ctx);
  return total;
}

} // namespace tsiemene
