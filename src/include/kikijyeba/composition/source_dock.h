// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "piaabo/parse/simple_kv_block.h"
#include "ujcamei/graph/graph.h"
#include "ujcamei/source/contract/contract.h"
#include "ujcamei/source/contract/dsl/channel_forms_decoder.h"
#include "ujcamei/source/contract/dsl/graph_forms_decoder.h"
#include "ujcamei/source/instrument_signature.h"

namespace cuwacunu::kikijyeba::composition {

using ujcamei_source_universe_t =
    cuwacunu::ujcamei::source::contract::source_universe_t;

struct graph_first_source_dock_paths_t {
  std::string channels_bnf_path{};
  std::string channels_dsl_path{};
  std::string graph_bnf_path{};
  std::string graph_dsl_path{};
};

enum class graph_first_edge_resolution_policy_t {
  explicit_only,
  edge_discovery,
};

enum class graph_first_fetch_mode_t {
  serial,
  parallel_by_edge,
};

// Kikijyeba graph-first dock: how this procedure asks Ujcamei sources to be
// sampled and arranged before Wikimyei workers consume them.
struct graph_first_source_dock_t {
  graph_first_edge_resolution_policy_t edge_resolution_policy{
      graph_first_edge_resolution_policy_t::explicit_only};
  std::string edge_source_kind{"real"};
  graph_first_fetch_mode_t fetch_mode{graph_first_fetch_mode_t::serial};
  std::int64_t max_fetch_workers{0};
  std::int64_t parallel_min_work_items{16};
  std::vector<cuwacunu::ujcamei::source::contract::channel_form_t>
      channel_forms{};
  std::vector<cuwacunu::ujcamei::source::contract::graph_node_form_t>
      graph_node_forms{};
  std::vector<cuwacunu::ujcamei::source::contract::graph_edge_form_t>
      graph_edge_forms{};

  [[nodiscard]] bool empty() const {
    return channel_forms.empty() && graph_node_forms.empty() &&
           graph_edge_forms.empty();
  }
};

struct graph_first_source_resolution_warning_t {
  std::string code{};
  std::string message{};
  std::string edge_id{};
  std::string reverse_edge_id{};
  std::string node_id{};
  std::string source_instrument{};
};

struct graph_first_source_resolution_report_t {
  std::string edge_resolution_policy{"explicit_only"};
  std::string edge_source_kind{"real"};
  std::string fetch_mode{"serial"};
  std::int64_t max_fetch_workers{0};
  std::int64_t parallel_min_work_items{16};
  std::int64_t active_node_count{0};
  std::int64_t possible_directed_edge_count{0};
  std::int64_t available_source_directed_edge_count{0};
  std::int64_t selected_edge_count{0};
  std::int64_t missing_directed_pair_count{0};
  std::int64_t available_unselected_edge_count{0};
  std::int64_t selected_missing_source_edge_count{0};
  std::int64_t reverse_pair_count{0};
  std::int64_t selected_missing_reverse_count{0};
  std::int64_t isolated_node_count{0};
  std::int64_t connected_component_count{0};
  std::int64_t selected_cycle_dimension{0};

  std::vector<std::string> active_node_ids{};
  std::vector<std::string> selected_edge_ids{};
  std::vector<std::string> available_source_edge_ids{};
  std::vector<std::string> missing_directed_pairs{};
  std::vector<std::string> available_unselected_edge_ids{};
  std::vector<std::string> selected_missing_source_edge_ids{};
  std::vector<std::string> isolated_node_ids{};
  std::vector<std::int64_t> in_degree{};
  std::vector<std::int64_t> out_degree{};
  std::vector<graph_first_source_resolution_warning_t> warnings{};

  [[nodiscard]] bool has_warnings() const { return !warnings.empty(); }

  [[nodiscard]] std::string summary() const {
    std::ostringstream oss;
    oss << "graph_first_source_resolution: edge_policy="
        << edge_resolution_policy << " edge_source_kind=" << edge_source_kind
        << " fetch_mode=" << fetch_mode
        << " max_fetch_workers=" << max_fetch_workers
        << " parallel_min_work_items=" << parallel_min_work_items
        << " active_nodes=" << active_node_count
        << " possible_directed_edges=" << possible_directed_edge_count
        << " available_source_edges=" << available_source_directed_edge_count
        << " selected_edges=" << selected_edge_count
        << " missing_directed_pairs=" << missing_directed_pair_count
        << " available_unselected_edges=" << available_unselected_edge_count
        << " selected_missing_source_edges="
        << selected_missing_source_edge_count
        << " reverse_pairs=" << reverse_pair_count
        << " missing_selected_reverses=" << selected_missing_reverse_count
        << " isolated_nodes=" << isolated_node_count
        << " connected_components=" << connected_component_count
        << " cycle_dimension=" << selected_cycle_dimension
        << " warnings=" << warnings.size();
    return oss.str();
  }
};

struct resolved_graph_first_source_plan_t {
  // Compatibility shape for lower Ujcamei storage code that still accepts one
  // source_spec_t. New composition code should read graph/dock identity from
  // the explicit fields below.
  cuwacunu::ujcamei::source::contract::source_spec_t compat_source_spec{};
  cuwacunu::ujcamei::graph::market_graph_t market_graph{};
  std::unordered_map<std::string,
                     cuwacunu::ujcamei::source::instrument_signature_t>
      edge_instruments{};
};

[[nodiscard]] inline bool
graph_first_dock_active_text(const std::string &value) {
  return cuwacunu::piaabo::parse::simple_kv::lowercase(
             cuwacunu::piaabo::parse::simple_kv::trim(value)) == "true";
}

[[nodiscard]] inline std::int64_t
graph_first_dock_parse_positive_i64(const std::string &value,
                                    const char *field) {
  const auto out = cuwacunu::piaabo::parse::simple_kv::parse_i64(value);
  if (out <= 0) {
    throw std::runtime_error(std::string("[graph_first_source_dock] ") + field +
                             " must be positive");
  }
  return out;
}

[[nodiscard]] inline std::string graph_first_edge_resolution_policy_name(
    graph_first_edge_resolution_policy_t policy) {
  switch (policy) {
  case graph_first_edge_resolution_policy_t::explicit_only:
    return "explicit_only";
  case graph_first_edge_resolution_policy_t::edge_discovery:
    return "edge_discovery";
  }
  return "unknown";
}

[[nodiscard]] inline graph_first_edge_resolution_policy_t
parse_graph_first_edge_resolution_policy(const std::string &value) {
  const auto normalized = cuwacunu::piaabo::parse::simple_kv::lowercase(
      cuwacunu::piaabo::parse::simple_kv::trim(value));
  if (normalized.empty() || normalized == "explicit_only") {
    return graph_first_edge_resolution_policy_t::explicit_only;
  }
  if (normalized == "edge_discovery") {
    return graph_first_edge_resolution_policy_t::edge_discovery;
  }
  throw std::runtime_error("[graph_first_source_dock] unknown "
                           "EDGE_RESOLUTION_POLICY: " +
                           value);
}

[[nodiscard]] inline std::string
graph_first_fetch_mode_name(graph_first_fetch_mode_t mode) {
  switch (mode) {
  case graph_first_fetch_mode_t::serial:
    return "serial";
  case graph_first_fetch_mode_t::parallel_by_edge:
    return "parallel_by_edge";
  }
  return "unknown";
}

[[nodiscard]] inline graph_first_fetch_mode_t
parse_graph_first_fetch_mode(const std::string &value) {
  const auto normalized = cuwacunu::piaabo::parse::simple_kv::lowercase(
      cuwacunu::piaabo::parse::simple_kv::trim(value));
  if (normalized.empty() || normalized == "serial") {
    return graph_first_fetch_mode_t::serial;
  }
  if (normalized == "parallel_by_edge") {
    return graph_first_fetch_mode_t::parallel_by_edge;
  }
  throw std::runtime_error("[graph_first_source_dock] unknown FETCH_MODE: " +
                           value);
}

[[nodiscard]] inline std::int64_t
parse_graph_first_nonnegative_i64(const std::string &value, const char *field) {
  const auto parsed = cuwacunu::piaabo::parse::simple_kv::parse_i64(value);
  if (parsed < 0) {
    throw std::runtime_error(std::string("[graph_first_source_dock] ") + field +
                             " must be nonnegative");
  }
  return parsed;
}

[[nodiscard]] inline std::int64_t parse_graph_first_positive_i64_or_default(
    const std::string &value, const char *field, std::int64_t default_value) {
  const auto trimmed = cuwacunu::piaabo::parse::simple_kv::trim(value);
  if (trimmed.empty()) {
    return default_value;
  }
  const auto parsed = cuwacunu::piaabo::parse::simple_kv::parse_i64(trimmed);
  if (parsed <= 0) {
    throw std::runtime_error(std::string("[graph_first_source_dock] ") + field +
                             " must be positive");
  }
  return parsed;
}

[[nodiscard]] inline std::string
parse_graph_first_edge_source_kind(const std::string &value) {
  const auto normalized = cuwacunu::piaabo::parse::simple_kv::lowercase(
      cuwacunu::piaabo::parse::simple_kv::trim(value));
  if (normalized.empty() || normalized == "real") {
    return "real";
  }
  if (normalized == "synthetic" || normalized == "derived") {
    return normalized;
  }
  throw std::runtime_error(
      "[graph_first_source_dock] unknown EDGE_SOURCE_KIND: " + value);
}

[[nodiscard]] inline ujcamei_source_universe_t make_ujcamei_source_universe(
    const cuwacunu::ujcamei::source::contract::source_spec_t &compat) {
  return cuwacunu::ujcamei::source::contract::make_source_universe_from_compat(
      compat);
}

[[nodiscard]] inline graph_first_source_dock_t make_graph_first_source_dock(
    const cuwacunu::ujcamei::source::contract::source_spec_t &compat) {
  graph_first_source_dock_t out{};
  out.edge_resolution_policy = parse_graph_first_edge_resolution_policy(
      compat.graph_edge_resolution_policy);
  out.edge_source_kind =
      parse_graph_first_edge_source_kind(compat.graph_edge_source_kind);
  out.fetch_mode = parse_graph_first_fetch_mode(compat.graph_fetch_mode);
  out.max_fetch_workers = parse_graph_first_nonnegative_i64(
      compat.graph_max_fetch_workers, "MAX_FETCH_WORKERS");
  out.parallel_min_work_items = parse_graph_first_positive_i64_or_default(
      compat.graph_parallel_min_work_items, "PARALLEL_MIN_WORK_ITEMS", 16);
  out.channel_forms = compat.channel_forms;
  out.graph_node_forms = compat.graph_node_forms;
  out.graph_edge_forms = compat.graph_edge_forms;
  return out;
}

[[nodiscard]] inline graph_first_source_dock_t
decode_graph_first_source_dock_from_split_dsl(std::string channel_grammar,
                                              std::string channel_instruction,
                                              std::string graph_grammar,
                                              std::string graph_instruction) {
  if (cuwacunu::piaabo::parse::simple_kv::trim(channel_grammar).empty() ||
      cuwacunu::piaabo::parse::simple_kv::trim(channel_instruction).empty()) {
    throw std::runtime_error(
        "[graph_first_source_dock] channel dock DSL is required");
  }
  cuwacunu::ujcamei::source::contract::dsl::channel_forms_decoder_t
      channels_decoder(std::move(channel_grammar));
  auto channels_part = channels_decoder.decode(std::move(channel_instruction));

  cuwacunu::ujcamei::source::contract::source_spec_t graph_part{};
  if (!cuwacunu::piaabo::parse::simple_kv::trim(graph_grammar).empty() ||
      !cuwacunu::piaabo::parse::simple_kv::trim(graph_instruction).empty()) {
    if (cuwacunu::piaabo::parse::simple_kv::trim(graph_grammar).empty() ||
        cuwacunu::piaabo::parse::simple_kv::trim(graph_instruction).empty()) {
      throw std::runtime_error(
          "[graph_first_source_dock] graph dock DSL requires both grammar and "
          "instruction text");
    }
    cuwacunu::ujcamei::source::contract::dsl::graph_forms_decoder_t
        graph_decoder(std::move(graph_grammar));
    graph_part = graph_decoder.decode(std::move(graph_instruction));
  }

  graph_first_source_dock_t out{};
  out.edge_resolution_policy = parse_graph_first_edge_resolution_policy(
      graph_part.graph_edge_resolution_policy);
  out.edge_source_kind =
      parse_graph_first_edge_source_kind(graph_part.graph_edge_source_kind);
  out.fetch_mode = parse_graph_first_fetch_mode(graph_part.graph_fetch_mode);
  out.max_fetch_workers = parse_graph_first_nonnegative_i64(
      graph_part.graph_max_fetch_workers, "MAX_FETCH_WORKERS");
  out.parallel_min_work_items = parse_graph_first_positive_i64_or_default(
      graph_part.graph_parallel_min_work_items, "PARALLEL_MIN_WORK_ITEMS", 16);
  out.channel_forms = std::move(channels_part.channel_forms);
  out.graph_node_forms = std::move(graph_part.graph_node_forms);
  out.graph_edge_forms = std::move(graph_part.graph_edge_forms);
  return out;
}

namespace graph_first_source_resolution_detail {

struct edge_candidate_key_t {
  std::string source_instrument{};
  std::string base_node{};
  std::string quote_node{};

  [[nodiscard]] bool operator<(const edge_candidate_key_t &other) const {
    return std::tie(source_instrument, base_node, quote_node) <
           std::tie(other.source_instrument, other.base_node, other.quote_node);
  }
};

struct selected_edge_t {
  std::string edge_id{};
  std::string source_instrument{};
  std::string base_node{};
  std::string quote_node{};
};

struct dsu_t {
  std::vector<std::int64_t> parent{};
  std::vector<std::int64_t> rank{};

  explicit dsu_t(std::size_t n) : parent(n), rank(n, 0) {
    for (std::size_t i = 0; i < n; ++i) {
      parent[i] = static_cast<std::int64_t>(i);
    }
  }

  [[nodiscard]] std::int64_t find(std::int64_t x) {
    auto idx = static_cast<std::size_t>(x);
    if (parent[idx] != x) {
      parent[idx] = find(parent[idx]);
    }
    return parent[idx];
  }

  void unite(std::int64_t a, std::int64_t b) {
    a = find(a);
    b = find(b);
    if (a == b) {
      return;
    }
    auto ai = static_cast<std::size_t>(a);
    auto bi = static_cast<std::size_t>(b);
    if (rank[ai] < rank[bi]) {
      std::swap(a, b);
      std::swap(ai, bi);
    }
    parent[bi] = a;
    if (rank[ai] == rank[bi]) {
      ++rank[ai];
    }
  }
};

[[nodiscard]] inline std::string edge_pair_label(const std::string &base,
                                                 const std::string &quote) {
  return base + "->" + quote;
}

[[nodiscard]] inline std::string
candidate_label(const edge_candidate_key_t &key) {
  return key.source_instrument + ":" +
         edge_pair_label(key.base_node, key.quote_node);
}

[[nodiscard]] inline std::vector<
    const cuwacunu::ujcamei::source::contract::channel_form_t *>
active_channels(const graph_first_source_dock_t &dock) {
  std::vector<const cuwacunu::ujcamei::source::contract::channel_form_t *> out;
  out.reserve(dock.channel_forms.size());
  for (const auto &channel : dock.channel_forms) {
    if (graph_first_dock_active_text(channel.active)) {
      out.push_back(&channel);
    }
  }
  return out;
}

[[nodiscard]] inline std::string channel_label(
    const cuwacunu::ujcamei::source::contract::channel_form_t &channel) {
  return cuwacunu::ujcamei::source::types::enum_to_string(channel.interval) +
         "/" + channel.record_type;
}

inline void add_warning(graph_first_source_resolution_report_t &report,
                        std::string code, std::string message,
                        std::string edge_id = {},
                        std::string reverse_edge_id = {},
                        std::string node_id = {},
                        std::string source_instrument = {}) {
  report.warnings.push_back(graph_first_source_resolution_warning_t{
      .code = std::move(code),
      .message = std::move(message),
      .edge_id = std::move(edge_id),
      .reverse_edge_id = std::move(reverse_edge_id),
      .node_id = std::move(node_id),
      .source_instrument = std::move(source_instrument),
  });
}

} // namespace graph_first_source_resolution_detail

[[nodiscard]] inline graph_first_source_resolution_report_t
analyze_graph_first_source_resolution(const ujcamei_source_universe_t &universe,
                                      const graph_first_source_dock_t &dock) {
  namespace detail = graph_first_source_resolution_detail;

  graph_first_source_resolution_report_t report{};
  report.edge_resolution_policy =
      graph_first_edge_resolution_policy_name(dock.edge_resolution_policy);
  report.edge_source_kind = dock.edge_source_kind;
  report.fetch_mode = graph_first_fetch_mode_name(dock.fetch_mode);
  report.max_fetch_workers = dock.max_fetch_workers;
  report.parallel_min_work_items = dock.parallel_min_work_items;

  std::unordered_map<std::string, std::size_t> node_to_index;
  for (const auto &node : dock.graph_node_forms) {
    if (!graph_first_dock_active_text(node.active)) {
      continue;
    }
    if (node_to_index.find(node.node_id) != node_to_index.end()) {
      detail::add_warning(report, "duplicate_active_node",
                          "duplicate active node id in graph dock", {}, {},
                          node.node_id);
      continue;
    }
    node_to_index.emplace(node.node_id, report.active_node_ids.size());
    report.active_node_ids.push_back(node.node_id);
  }
  report.active_node_count =
      static_cast<std::int64_t>(report.active_node_ids.size());
  report.possible_directed_edge_count =
      report.active_node_count *
      std::max<std::int64_t>(0, report.active_node_count - 1);
  report.in_degree.assign(report.active_node_ids.size(), 0);
  report.out_degree.assign(report.active_node_ids.size(), 0);

  if (report.active_node_ids.empty()) {
    detail::add_warning(report, "empty_active_node_set",
                        "graph-first dock has no active nodes");
  }

  const auto active_channels = detail::active_channels(dock);
  if (active_channels.empty()) {
    detail::add_warning(report, "empty_active_channel_set",
                        "graph-first dock has no active channels");
  }

  std::map<detail::edge_candidate_key_t, std::set<std::string>> coverage;
  std::map<std::pair<std::string, std::string>,
           std::set<detail::edge_candidate_key_t>>
      available_by_pair;
  for (const auto &source : universe.source_forms) {
    if (source.source_kind != dock.edge_source_kind || source.source.empty()) {
      continue;
    }
    if (node_to_index.find(source.base_asset) == node_to_index.end() ||
        node_to_index.find(source.quote_asset) == node_to_index.end() ||
        source.base_asset == source.quote_asset) {
      continue;
    }
    for (const auto *channel : active_channels) {
      if (source.interval == channel->interval &&
          source.record_type == channel->record_type) {
        const detail::edge_candidate_key_t key{
            .source_instrument = source.instrument,
            .base_node = source.base_asset,
            .quote_node = source.quote_asset,
        };
        coverage[key].insert(detail::channel_label(*channel));
      }
    }
  }

  const auto required_channel_count = active_channels.size();
  std::set<detail::edge_candidate_key_t> available_candidates;
  for (const auto &[key, channel_coverage] : coverage) {
    if (required_channel_count > 0 &&
        channel_coverage.size() == required_channel_count) {
      available_candidates.insert(key);
      available_by_pair[{key.base_node, key.quote_node}].insert(key);
      report.available_source_edge_ids.push_back(detail::candidate_label(key));
    }
  }
  report.available_source_directed_edge_count =
      static_cast<std::int64_t>(available_candidates.size());
  std::sort(report.available_source_edge_ids.begin(),
            report.available_source_edge_ids.end());

  for (const auto &base : report.active_node_ids) {
    for (const auto &quote : report.active_node_ids) {
      if (base == quote) {
        continue;
      }
      if (available_by_pair.find({base, quote}) == available_by_pair.end()) {
        report.missing_directed_pairs.push_back(
            detail::edge_pair_label(base, quote));
      }
    }
  }
  report.missing_directed_pair_count =
      static_cast<std::int64_t>(report.missing_directed_pairs.size());
  if (!report.missing_directed_pairs.empty()) {
    detail::add_warning(
        report, "missing_available_directed_pairs",
        "some active node ordered pairs have no real source coverage for all "
        "active channels");
  }

  std::vector<detail::selected_edge_t> selected_edges;
  std::map<std::pair<std::string, std::string>, std::vector<std::size_t>>
      selected_by_pair;
  std::set<detail::edge_candidate_key_t> selected_candidates;
  std::unordered_set<std::string> seen_selected_edge_ids;
  for (const auto &edge : dock.graph_edge_forms) {
    if (!graph_first_dock_active_text(edge.active)) {
      continue;
    }
    if (!seen_selected_edge_ids.insert(edge.edge_id).second) {
      detail::add_warning(report, "duplicate_selected_edge",
                          "duplicate selected edge id in graph dock",
                          edge.edge_id);
      continue;
    }
    report.selected_edge_ids.push_back(edge.edge_id);
    const bool base_active =
        node_to_index.find(edge.base_node) != node_to_index.end();
    const bool quote_active =
        node_to_index.find(edge.quote_node) != node_to_index.end();
    if (!base_active || !quote_active || edge.base_node == edge.quote_node) {
      detail::add_warning(report, "invalid_selected_edge_endpoints",
                          "selected edge endpoints are not valid active nodes",
                          edge.edge_id);
      continue;
    }
    const std::string source_instrument =
        edge.source_instrument.empty() ? edge.edge_id : edge.source_instrument;
    const detail::selected_edge_t selected{
        .edge_id = edge.edge_id,
        .source_instrument = source_instrument,
        .base_node = edge.base_node,
        .quote_node = edge.quote_node,
    };
    const detail::edge_candidate_key_t candidate{
        .source_instrument = source_instrument,
        .base_node = edge.base_node,
        .quote_node = edge.quote_node,
    };
    if (available_candidates.find(candidate) == available_candidates.end()) {
      report.selected_missing_source_edge_ids.push_back(edge.edge_id);
      detail::add_warning(report, "selected_edge_missing_source_coverage",
                          "selected edge does not have full real source "
                          "coverage for all active channels",
                          edge.edge_id, {}, {}, source_instrument);
    } else {
      selected_candidates.insert(candidate);
    }
    const auto idx = selected_edges.size();
    selected_edges.push_back(selected);
    selected_by_pair[{selected.base_node, selected.quote_node}].push_back(idx);
    ++report.out_degree[node_to_index.at(selected.base_node)];
    ++report.in_degree[node_to_index.at(selected.quote_node)];
  }
  report.selected_edge_count = static_cast<std::int64_t>(selected_edges.size());
  report.selected_missing_source_edge_count =
      static_cast<std::int64_t>(report.selected_missing_source_edge_ids.size());

  for (const auto &candidate : available_candidates) {
    if (selected_candidates.find(candidate) == selected_candidates.end()) {
      report.available_unselected_edge_ids.push_back(
          detail::candidate_label(candidate));
    }
  }
  report.available_unselected_edge_count =
      static_cast<std::int64_t>(report.available_unselected_edge_ids.size());
  if (!report.available_unselected_edge_ids.empty()) {
    detail::add_warning(
        report, "available_unselected_edges",
        "real source edges exist for active nodes but are not selected by the "
        "graph dock");
  }

  for (std::size_t i = 0; i < selected_edges.size(); ++i) {
    const auto &edge = selected_edges[i];
    const auto reverse =
        selected_by_pair.find({edge.quote_node, edge.base_node});
    if (reverse == selected_by_pair.end() || reverse->second.empty()) {
      ++report.selected_missing_reverse_count;
      continue;
    }
    for (const auto reverse_index : reverse->second) {
      if (i < reverse_index) {
        ++report.reverse_pair_count;
      }
    }
  }

  if (!report.active_node_ids.empty()) {
    detail::dsu_t dsu(report.active_node_ids.size());
    std::int64_t valid_undirected_edge_count = 0;
    for (const auto &edge : selected_edges) {
      const auto base_it = node_to_index.find(edge.base_node);
      const auto quote_it = node_to_index.find(edge.quote_node);
      if (base_it == node_to_index.end() || quote_it == node_to_index.end()) {
        continue;
      }
      dsu.unite(static_cast<std::int64_t>(base_it->second),
                static_cast<std::int64_t>(quote_it->second));
      ++valid_undirected_edge_count;
    }
    std::unordered_map<std::int64_t, std::int64_t> nodes_per_component;
    for (std::size_t n = 0; n < report.active_node_ids.size(); ++n) {
      ++nodes_per_component[dsu.find(static_cast<std::int64_t>(n))];
      if (report.in_degree[n] == 0 && report.out_degree[n] == 0) {
        ++report.isolated_node_count;
        report.isolated_node_ids.push_back(report.active_node_ids[n]);
        detail::add_warning(report, "isolated_active_node",
                            "active node has no selected incident edge", {}, {},
                            report.active_node_ids[n]);
      } else if (report.in_degree[n] == 0 || report.out_degree[n] == 0) {
        detail::add_warning(report, "directionally_unbalanced_node",
                            "active node has selected edges only on one "
                            "directed side",
                            {}, {}, report.active_node_ids[n]);
      }
    }
    report.connected_component_count =
        static_cast<std::int64_t>(nodes_per_component.size());
    report.selected_cycle_dimension = std::max<std::int64_t>(
        0, valid_undirected_edge_count - report.active_node_count +
               report.connected_component_count);
  }

  return report;
}

[[nodiscard]] inline std::vector<
    cuwacunu::ujcamei::source::contract::graph_edge_form_t>
discover_graph_edge_forms(const ujcamei_source_universe_t &universe,
                          const graph_first_source_dock_t &dock) {
  namespace detail = graph_first_source_resolution_detail;

  std::unordered_set<std::string> active_nodes;
  active_nodes.reserve(dock.graph_node_forms.size());
  for (const auto &node : dock.graph_node_forms) {
    if (graph_first_dock_active_text(node.active)) {
      active_nodes.insert(node.node_id);
    }
  }

  const auto active_channels = detail::active_channels(dock);
  std::map<detail::edge_candidate_key_t, std::set<std::string>> coverage;
  for (const auto &source : universe.source_forms) {
    if (source.source_kind != dock.edge_source_kind || source.source.empty()) {
      continue;
    }
    if (source.base_asset == source.quote_asset ||
        active_nodes.find(source.base_asset) == active_nodes.end() ||
        active_nodes.find(source.quote_asset) == active_nodes.end()) {
      continue;
    }
    for (const auto *channel : active_channels) {
      if (source.interval == channel->interval &&
          source.record_type == channel->record_type) {
        const detail::edge_candidate_key_t key{
            .source_instrument = source.instrument,
            .base_node = source.base_asset,
            .quote_node = source.quote_asset,
        };
        coverage[key].insert(detail::channel_label(*channel));
      }
    }
  }

  std::vector<detail::edge_candidate_key_t> candidates;
  const auto required_channel_count = active_channels.size();
  for (const auto &[key, channel_coverage] : coverage) {
    if (required_channel_count > 0 &&
        channel_coverage.size() == required_channel_count) {
      candidates.push_back(key);
    }
  }
  std::sort(candidates.begin(), candidates.end(),
            [](const detail::edge_candidate_key_t &a,
               const detail::edge_candidate_key_t &b) {
              return std::tie(a.base_node, a.quote_node, a.source_instrument) <
                     std::tie(b.base_node, b.quote_node, b.source_instrument);
            });

  std::vector<cuwacunu::ujcamei::source::contract::graph_edge_form_t> out;
  out.reserve(candidates.size());
  for (const auto &candidate : candidates) {
    out.push_back(cuwacunu::ujcamei::source::contract::graph_edge_form_t{
        .edge_id = candidate.source_instrument,
        .base_node = candidate.base_node,
        .quote_node = candidate.quote_node,
        .source_instrument = candidate.source_instrument,
        .active = "true",
    });
  }
  return out;
}

[[nodiscard]] inline graph_first_source_dock_t
resolve_graph_first_source_dock_edges(const ujcamei_source_universe_t &universe,
                                      const graph_first_source_dock_t &dock) {
  if (dock.edge_resolution_policy ==
      graph_first_edge_resolution_policy_t::explicit_only) {
    return dock;
  }

  graph_first_source_dock_t out = dock;
  out.graph_edge_forms = discover_graph_edge_forms(universe, dock);
  if (out.graph_edge_forms.empty()) {
    throw std::runtime_error(
        "[graph_first_source_dock] edge_discovery resolved no active graph "
        "edges for the active nodes/channels/source-kind policy");
  }
  return out;
}

[[nodiscard]] inline cuwacunu::ujcamei::source::contract::source_spec_t
make_graph_first_compat_source_spec(const ujcamei_source_universe_t &universe,
                                    const graph_first_source_dock_t &dock) {
  cuwacunu::ujcamei::source::contract::source_spec_t out{};
  out.source_forms = universe.source_forms;
  out.graph_edge_resolution_policy =
      graph_first_edge_resolution_policy_name(dock.edge_resolution_policy);
  out.graph_edge_source_kind = dock.edge_source_kind;
  out.graph_fetch_mode = graph_first_fetch_mode_name(dock.fetch_mode);
  out.graph_max_fetch_workers = std::to_string(dock.max_fetch_workers);
  out.graph_parallel_min_work_items =
      std::to_string(dock.parallel_min_work_items);
  out.channel_forms = dock.channel_forms;
  out.graph_node_forms = dock.graph_node_forms;
  out.graph_edge_forms = dock.graph_edge_forms;
  out.csv_bootstrap_deltas = universe.csv_bootstrap_deltas;
  out.csv_step_abs_tol = universe.csv_step_abs_tol;
  out.csv_step_rel_tol = universe.csv_step_rel_tol;
  out.data_analytics_policy = universe.data_analytics_policy;
  return out;
}

[[nodiscard]] inline resolved_graph_first_source_plan_t
resolve_graph_first_source_plan(const ujcamei_source_universe_t &universe,
                                const graph_first_source_dock_t &dock) {
  if (universe.source_forms.empty()) {
    throw std::runtime_error(
        "[graph_first_source_dock] Ujcamei source universe is empty");
  }
  if (dock.channel_forms.empty()) {
    throw std::runtime_error(
        "[graph_first_source_dock] graph-first channel dock is empty");
  }
  const auto resolved_dock =
      resolve_graph_first_source_dock_edges(universe, dock);
  if (resolved_dock.graph_node_forms.empty() ||
      resolved_dock.graph_edge_forms.empty()) {
    throw std::runtime_error(
        "[graph_first_source_dock] graph-first graph dock is empty");
  }

  resolved_graph_first_source_plan_t out{};
  out.compat_source_spec =
      make_graph_first_compat_source_spec(universe, resolved_dock);
  out.market_graph = out.compat_source_spec.active_market_graph();
  out.edge_instruments = out.compat_source_spec.active_edge_instrument_map();
  return out;
}

[[nodiscard]] inline std::int64_t
active_channel_count(const graph_first_source_dock_t &dock) {
  std::int64_t out = 0;
  for (const auto &channel : dock.channel_forms) {
    if (graph_first_dock_active_text(channel.active)) {
      ++out;
    }
  }
  return out;
}

[[nodiscard]] inline std::int64_t
max_input_length(const graph_first_source_dock_t &dock) {
  std::int64_t out = 0;
  for (const auto &channel : dock.channel_forms) {
    if (graph_first_dock_active_text(channel.active)) {
      out = std::max(out, graph_first_dock_parse_positive_i64(
                              channel.input_length, "input_length"));
    }
  }
  return out;
}

[[nodiscard]] inline std::int64_t
max_future_length(const graph_first_source_dock_t &dock) {
  std::int64_t out = 0;
  for (const auto &channel : dock.channel_forms) {
    if (graph_first_dock_active_text(channel.active)) {
      out = std::max(out, graph_first_dock_parse_positive_i64(
                              channel.future_length, "future_length"));
    }
  }
  return out;
}

} // namespace cuwacunu::kikijyeba::composition
