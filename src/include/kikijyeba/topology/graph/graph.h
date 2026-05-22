// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "kikijyeba/topology/graph/edge.h"

namespace cuwacunu::kikijyeba::topology::graph {

namespace detail {

inline constexpr std::uint64_t kFnvOffsetBasis = 14695981039346656037ull;
inline constexpr std::uint64_t kFnvPrime = 1099511628211ull;

inline void mix_graph_fingerprint_byte(std::uint64_t &hash,
                                       unsigned char byte) {
  hash ^= static_cast<std::uint64_t>(byte);
  hash *= kFnvPrime;
}

inline void mix_graph_fingerprint_string(std::uint64_t &hash,
                                         std::string_view value) {
  for (const unsigned char c : value) {
    mix_graph_fingerprint_byte(hash, c);
  }
  mix_graph_fingerprint_byte(hash, 0xffu);
}

inline void mix_graph_fingerprint_i64(std::uint64_t &hash, std::int64_t value) {
  for (int i = 0; i < 8; ++i) {
    mix_graph_fingerprint_byte(
        hash, static_cast<unsigned char>((static_cast<std::uint64_t>(value) >>
                                          (static_cast<unsigned>(i) * 8u)) &
                                         0xffu));
  }
  mix_graph_fingerprint_byte(hash, 0xfeu);
}

[[nodiscard]] inline std::string graph_fingerprint_hex(std::uint64_t hash) {
  std::ostringstream oss;
  oss << std::hex << std::setfill('0') << std::setw(16) << hash;
  return oss.str();
}

} // namespace detail

[[nodiscard]] inline std::string compute_graph_order_fingerprint(
    const std::vector<asset_node_id_t> &node_ids,
    const std::vector<instrument_edge_id_t> &edge_ids,
    const std::vector<node_index_t> &base_index,
    const std::vector<node_index_t> &quote_index) {
  std::uint64_t hash = detail::kFnvOffsetBasis;
  detail::mix_graph_fingerprint_string(hash, "cuwacunu.graph.order.v1");
  detail::mix_graph_fingerprint_i64(hash,
                                    static_cast<std::int64_t>(node_ids.size()));
  for (const auto &node_id : node_ids) {
    detail::mix_graph_fingerprint_string(hash, node_id);
  }
  detail::mix_graph_fingerprint_i64(hash,
                                    static_cast<std::int64_t>(edge_ids.size()));
  for (const auto &edge_id : edge_ids) {
    detail::mix_graph_fingerprint_string(hash, edge_id);
  }
  detail::mix_graph_fingerprint_i64(
      hash, static_cast<std::int64_t>(base_index.size()));
  for (const auto node_index : base_index) {
    detail::mix_graph_fingerprint_i64(hash, node_index);
  }
  detail::mix_graph_fingerprint_i64(
      hash, static_cast<std::int64_t>(quote_index.size()));
  for (const auto node_index : quote_index) {
    detail::mix_graph_fingerprint_i64(hash, node_index);
  }
  return detail::graph_fingerprint_hex(hash);
}

struct market_graph_validation_options_t {
  bool allow_duplicate_edge_ids{false};
};

struct market_graph_t {
  std::vector<asset_node_id_t> node_ids{};
  std::vector<instrument_edge_id_t> edge_ids{};
  std::vector<node_index_t> base_index{};
  std::vector<node_index_t> quote_index{};

  [[nodiscard]] node_index_t num_nodes() const {
    return static_cast<node_index_t>(node_ids.size());
  }

  [[nodiscard]] edge_index_t num_edges() const {
    return static_cast<edge_index_t>(edge_ids.size());
  }

  void validate(market_graph_validation_options_t options = {}) const {
    if (node_ids.empty()) {
      throw std::invalid_argument(
          "[market_graph_t] node_ids must not be empty");
    }
    if (edge_ids.empty()) {
      throw std::invalid_argument(
          "[market_graph_t] edge_ids must not be empty");
    }
    if (base_index.size() != edge_ids.size() ||
        quote_index.size() != edge_ids.size()) {
      throw std::invalid_argument(
          "[market_graph_t] edge_ids/base_index/quote_index size mismatch");
    }

    std::unordered_set<std::string> seen_nodes;
    seen_nodes.reserve(node_ids.size());
    for (const auto &node_id : node_ids) {
      if (node_id.empty()) {
        throw std::invalid_argument(
            "[market_graph_t] node_ids must not contain empty ids");
      }
      if (!seen_nodes.insert(node_id).second) {
        throw std::invalid_argument("[market_graph_t] duplicate node_id: " +
                                    node_id);
      }
    }

    std::unordered_set<std::string> seen_edges;
    seen_edges.reserve(edge_ids.size());
    for (std::size_t e = 0; e < edge_ids.size(); ++e) {
      if (edge_ids[e].empty()) {
        throw std::invalid_argument(
            "[market_graph_t] edge_ids must not contain empty ids");
      }
      if (!options.allow_duplicate_edge_ids &&
          !seen_edges.insert(edge_ids[e]).second) {
        throw std::invalid_argument("[market_graph_t] duplicate edge_id: " +
                                    edge_ids[e]);
      }
      const auto u = base_index[e];
      const auto v = quote_index[e];
      if (u < 0 || v < 0 || u >= num_nodes() || v >= num_nodes()) {
        throw std::invalid_argument("[market_graph_t] edge endpoint index out "
                                    "of node range for edge_id: " +
                                    edge_ids[e]);
      }
      if (u == v) {
        throw std::invalid_argument(
            "[market_graph_t] self edge is not valid for edge_id: " +
            edge_ids[e]);
      }
    }
  }

  void validate(bool allow_duplicate_edge_ids) const {
    validate(market_graph_validation_options_t{
        .allow_duplicate_edge_ids = allow_duplicate_edge_ids,
    });
  }

  [[nodiscard]] std::string computed_graph_order_fingerprint() const {
    return compute_graph_order_fingerprint(node_ids, edge_ids, base_index,
                                           quote_index);
  }

  [[nodiscard]] node_index_t
  node_index_or_throw(std::string_view node_id) const {
    for (node_index_t i = 0; i < num_nodes(); ++i) {
      if (node_ids[static_cast<std::size_t>(i)] == node_id) {
        return i;
      }
    }
    throw std::out_of_range("[market_graph_t] unknown node_id: " +
                            std::string(node_id));
  }

  [[nodiscard]] edge_index_t
  edge_index_or_throw(std::string_view edge_id) const {
    for (edge_index_t e = 0; e < num_edges(); ++e) {
      if (edge_ids[static_cast<std::size_t>(e)] == edge_id) {
        return e;
      }
    }
    throw std::out_of_range("[market_graph_t] unknown edge_id: " +
                            std::string(edge_id));
  }

  [[nodiscard]] asset_node_id_t base_node_id(edge_index_t edge_index) const {
    return node_ids[static_cast<std::size_t>(
        endpoint_index_or_throw(edge_index, endpoint_role_t::base))];
  }

  [[nodiscard]] asset_node_id_t quote_node_id(edge_index_t edge_index) const {
    return node_ids[static_cast<std::size_t>(
        endpoint_index_or_throw(edge_index, endpoint_role_t::quote))];
  }

  [[nodiscard]] directed_edge_t directed_edge(edge_index_t edge_index) const {
    check_edge_index(edge_index);
    const auto e = static_cast<std::size_t>(edge_index);
    return directed_edge_t{
        .edge_id = edge_ids[e],
        .base_node_id = base_node_id(edge_index),
        .quote_node_id = quote_node_id(edge_index),
    };
  }

  [[nodiscard]] directed_edge_t directed_edge(std::string_view edge_id) const {
    return directed_edge(edge_index_or_throw(edge_id));
  }

  [[nodiscard]] bool has_reverse_edge(edge_index_t edge_index) const {
    const auto base =
        endpoint_index_or_throw(edge_index, endpoint_role_t::base);
    const auto quote =
        endpoint_index_or_throw(edge_index, endpoint_role_t::quote);
    for (edge_index_t e = 0; e < num_edges(); ++e) {
      if (base_index[static_cast<std::size_t>(e)] == quote &&
          quote_index[static_cast<std::size_t>(e)] == base) {
        return true;
      }
    }
    return false;
  }

private:
  void check_edge_index(edge_index_t edge_index) const {
    if (edge_index < 0 || edge_index >= num_edges()) {
      throw std::out_of_range("[market_graph_t] edge index out of range");
    }
  }

  [[nodiscard]] node_index_t
  endpoint_index_or_throw(edge_index_t edge_index, endpoint_role_t role) const {
    check_edge_index(edge_index);
    const auto e = static_cast<std::size_t>(edge_index);
    const node_index_t node_index =
        (role == endpoint_role_t::base) ? base_index[e] : quote_index[e];
    if (node_index < 0 || node_index >= num_nodes()) {
      throw std::out_of_range("[market_graph_t] endpoint index out of range");
    }
    return node_index;
  }
};

[[nodiscard]] inline market_graph_t
make_market_graph(const std::vector<directed_instrument_edge_t> &edges) {
  market_graph_t graph{};
  std::unordered_map<std::string, node_index_t> node_to_index;
  node_to_index.reserve(edges.size() * 2);

  auto intern_node = [&](std::string_view id) -> node_index_t {
    const std::string key{id};
    const auto found = node_to_index.find(key);
    if (found != node_to_index.end()) {
      return found->second;
    }
    const auto index = static_cast<node_index_t>(graph.node_ids.size());
    graph.node_ids.push_back(key);
    node_to_index.emplace(key, index);
    return index;
  };

  graph.edge_ids.reserve(edges.size());
  graph.base_index.reserve(edges.size());
  graph.quote_index.reserve(edges.size());
  for (const auto &edge : edges) {
    graph.edge_ids.push_back(edge.edge_id);
    graph.base_index.push_back(intern_node(edge.base_node_id));
    graph.quote_index.push_back(intern_node(edge.quote_node_id));
  }

  graph.validate();
  return graph;
}

} // namespace cuwacunu::kikijyeba::topology::graph
