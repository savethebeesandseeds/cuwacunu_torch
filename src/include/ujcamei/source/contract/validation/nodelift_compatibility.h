// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <iomanip>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "kikijyeba/topology/graph/graph.h"
#include "ujcamei/source/contract/contract.h"
#include "ujcamei/source/registry/instrument_signature.h"
#include "ujcamei/source/registry/types/kline_feature_registry.h"
#include "ujcamei/source/registry/types/enums.h"

namespace cuwacunu::ujcamei::source::contract::validation {

enum class validation_severity_t {
  info,
  warning,
  error,
};

enum class validation_code_t {
  duplicate_node_id,
  duplicate_edge_id,
  self_edge,
  invalid_node_index,
  isolated_node,
  reverse_pair_real,
  reverse_pair_unproven,
  reverse_pair_synthetic,
  missing_edge_dataset,
  missing_source_row,
  ambiguous_source_signature,
  graph_source_base_quote_mismatch,
  non_kline_channel,
  feature_width_mismatch,
  inactive_or_empty_channel_set,
  graph_fingerprint_mismatch,
  edge_id_symbol_mismatch,
  normalization_policy_mismatch,
  invalid_input_length,
  invalid_future_length,
};

enum class reverse_edge_policy_t {
  prove_or_fail,
  allow_without_proof_for_tests,
};

enum class source_validation_mode_t {
  none_for_tests,
  construction_only,
};

struct validation_issue_t {
  validation_severity_t severity{validation_severity_t::info};
  validation_code_t code{validation_code_t::duplicate_node_id};
  std::string message{};
  std::optional<std::string> edge_id{std::nullopt};
  std::optional<std::string> reverse_edge_id{std::nullopt};
  std::optional<std::string> node_id{std::nullopt};
  std::optional<std::string> channel_label{std::nullopt};
};

struct nodelift_compatibility_report_t {
  std::vector<validation_issue_t> issues{};
  std::int64_t node_count{0};
  std::int64_t edge_count{0};
  std::int64_t reverse_pair_count{0};
  std::int64_t isolated_node_count{0};

  [[nodiscard]] bool has_errors() const {
    return std::any_of(issues.begin(), issues.end(),
                       [](const validation_issue_t &issue) {
                         return issue.severity == validation_severity_t::error;
                       });
  }

  [[nodiscard]] bool ok() const { return !has_errors(); }

  [[nodiscard]] std::int64_t warning_count() const {
    return static_cast<std::int64_t>(std::count_if(
        issues.begin(), issues.end(), [](const validation_issue_t &issue) {
          return issue.severity == validation_severity_t::warning;
        }));
  }

  [[nodiscard]] bool has_code(validation_code_t code) const {
    return std::any_of(
        issues.begin(), issues.end(),
        [code](const validation_issue_t &issue) { return issue.code == code; });
  }

  [[nodiscard]] std::string summary() const;
};

struct nodelift_compatibility_identity_t {
  std::string graph_fingerprint{};
  std::string ujcamei_source_universe_hash{};
  std::string graph_first_channel_hash{};
  std::string graph_first_graph_hash{};
  std::string nodelift_version_token{"wikimyei.expression.nodelift.srl.v1"};
};

struct nodelift_compatibility_options_t {
  reverse_edge_policy_t reverse_edge_policy{
      reverse_edge_policy_t::prove_or_fail};
  source_validation_mode_t source_validation_mode{
      source_validation_mode_t::construction_only};
  bool require_kline_channels{true};
  bool require_log_returns_normalization{true};
  bool require_unique_source_per_edge_interval{true};
  bool warn_isolated_nodes{true};
  bool include_future{true};
  bool require_future{true};
  std::int64_t expected_feature_width{
      cuwacunu::ujcamei::source::registry::types::kKlineFeatureWidth};
};

[[nodiscard]] inline const char *
validation_severity_name(validation_severity_t severity) {
  switch (severity) {
  case validation_severity_t::info:
    return "info";
  case validation_severity_t::warning:
    return "warning";
  case validation_severity_t::error:
    return "error";
  }
  return "unknown";
}

[[nodiscard]] inline const char *validation_code_name(validation_code_t code) {
  switch (code) {
  case validation_code_t::duplicate_node_id:
    return "duplicate_node_id";
  case validation_code_t::duplicate_edge_id:
    return "duplicate_edge_id";
  case validation_code_t::self_edge:
    return "self_edge";
  case validation_code_t::invalid_node_index:
    return "invalid_node_index";
  case validation_code_t::isolated_node:
    return "isolated_node";
  case validation_code_t::reverse_pair_real:
    return "reverse_pair_real";
  case validation_code_t::reverse_pair_unproven:
    return "reverse_pair_unproven";
  case validation_code_t::reverse_pair_synthetic:
    return "reverse_pair_synthetic";
  case validation_code_t::missing_edge_dataset:
    return "missing_edge_dataset";
  case validation_code_t::missing_source_row:
    return "missing_source_row";
  case validation_code_t::ambiguous_source_signature:
    return "ambiguous_source_signature";
  case validation_code_t::graph_source_base_quote_mismatch:
    return "graph_source_base_quote_mismatch";
  case validation_code_t::non_kline_channel:
    return "non_kline_channel";
  case validation_code_t::feature_width_mismatch:
    return "feature_width_mismatch";
  case validation_code_t::inactive_or_empty_channel_set:
    return "inactive_or_empty_channel_set";
  case validation_code_t::graph_fingerprint_mismatch:
    return "graph_fingerprint_mismatch";
  case validation_code_t::edge_id_symbol_mismatch:
    return "edge_id_symbol_mismatch";
  case validation_code_t::normalization_policy_mismatch:
    return "normalization_policy_mismatch";
  case validation_code_t::invalid_input_length:
    return "invalid_input_length";
  case validation_code_t::invalid_future_length:
    return "invalid_future_length";
  }
  return "unknown";
}

inline std::string nodelift_compatibility_report_t::summary() const {
  std::ostringstream oss;
  oss << "NodeLift compatibility validation: nodes=" << node_count
      << " edges=" << edge_count << " reverse_pairs=" << reverse_pair_count
      << " isolated_nodes=" << isolated_node_count
      << " issues=" << issues.size();
  for (const auto &issue : issues) {
    oss << "\n  [" << validation_severity_name(issue.severity) << ":"
        << validation_code_name(issue.code) << "] " << issue.message;
    if (issue.edge_id.has_value()) {
      oss << " edge_id=" << *issue.edge_id;
    }
    if (issue.reverse_edge_id.has_value()) {
      oss << " reverse_edge_id=" << *issue.reverse_edge_id;
    }
    if (issue.node_id.has_value()) {
      oss << " node_id=" << *issue.node_id;
    }
    if (issue.channel_label.has_value()) {
      oss << " channel=" << *issue.channel_label;
    }
  }
  return oss.str();
}

namespace nodelift_compatibility_detail {

inline constexpr std::uint64_t kFnvOffsetBasis = 14695981039346656037ull;
inline constexpr std::uint64_t kFnvPrime = 1099511628211ull;

inline void mix_byte(std::uint64_t &hash, unsigned char byte) {
  hash ^= static_cast<std::uint64_t>(byte);
  hash *= kFnvPrime;
}

inline void mix_string(std::uint64_t &hash, std::string_view value) {
  for (const unsigned char c : value) {
    mix_byte(hash, c);
  }
  mix_byte(hash, 0xffu);
}

inline void mix_i64(std::uint64_t &hash, std::int64_t value) {
  for (int i = 0; i < 8; ++i) {
    mix_byte(hash,
             static_cast<unsigned char>((static_cast<std::uint64_t>(value) >>
                                         (static_cast<unsigned>(i) * 8u)) &
                                        0xffu));
  }
  mix_byte(hash, 0xfeu);
}

[[nodiscard]] inline std::string hash_hex(std::uint64_t hash) {
  std::ostringstream oss;
  oss << std::hex << std::setfill('0') << std::setw(16) << hash;
  return oss.str();
}

[[nodiscard]] inline std::string trim_ascii(std::string_view in) {
  std::size_t b = 0;
  while (b < in.size() &&
         std::isspace(static_cast<unsigned char>(in[b])) != 0) {
    ++b;
  }
  std::size_t e = in.size();
  while (e > b && std::isspace(static_cast<unsigned char>(in[e - 1])) != 0) {
    --e;
  }
  return std::string(in.substr(b, e - b));
}

inline void add_issue(nodelift_compatibility_report_t &report,
                      validation_severity_t severity, validation_code_t code,
                      std::string message,
                      std::optional<std::string> edge_id = std::nullopt,
                      std::optional<std::string> reverse_edge_id = std::nullopt,
                      std::optional<std::string> node_id = std::nullopt,
                      std::optional<std::string> channel_label = std::nullopt) {
  report.issues.push_back(validation_issue_t{
      .severity = severity,
      .code = code,
      .message = std::move(message),
      .edge_id = std::move(edge_id),
      .reverse_edge_id = std::move(reverse_edge_id),
      .node_id = std::move(node_id),
      .channel_label = std::move(channel_label),
  });
}

[[nodiscard]] inline std::string channel_label(
    const cuwacunu::ujcamei::source::contract::channel_form_t &channel) {
  return cuwacunu::ujcamei::source::registry::types::enum_to_string(
             channel.interval) +
         "/" + channel.record_type;
}

[[nodiscard]] inline bool parse_positive_i64(std::string_view value,
                                             std::int64_t *out) {
  try {
    std::size_t consumed = 0;
    const auto parsed = std::stoll(std::string(value), &consumed);
    if (consumed != std::string(value).size() || parsed <= 0) {
      return false;
    }
    if (out) {
      *out = parsed;
    }
    return true;
  } catch (const std::exception &) {
    return false;
  }
}

[[nodiscard]] inline std::vector<
    const cuwacunu::ujcamei::source::contract::channel_form_t *>
active_channels(
    const cuwacunu::ujcamei::source::contract::source_spec_t &source_spec) {
  std::vector<const cuwacunu::ujcamei::source::contract::channel_form_t *> out;
  out.reserve(source_spec.channel_forms.size());
  for (const auto &channel : source_spec.channel_forms) {
    if (trim_ascii(channel.active) == "true") {
      out.push_back(&channel);
    }
  }
  return out;
}

[[nodiscard]] inline std::string compute_ujcamei_source_universe_hash(
    const cuwacunu::ujcamei::source::contract::source_spec_t *source_spec) {
  if (!source_spec) {
    return "none";
  }
  std::uint64_t hash = kFnvOffsetBasis;
  mix_string(hash, "cuwacunu.ujcamei.source_universe.v1");
  std::vector<std::string> rows;
  rows.reserve(source_spec->source_forms.size());
  for (const auto &form : source_spec->source_forms) {
    std::ostringstream row;
    row << "source|" << form.instrument << "|"
        << cuwacunu::ujcamei::source::registry::types::enum_to_string(
               form.interval)
        << "|" << form.record_type << "|" << form.market_type << "|"
        << form.venue << "|" << form.base_asset << "|" << form.quote_asset
        << "|" << form.source_kind << "|" << form.source;
    rows.push_back(row.str());
  }
  std::sort(rows.begin(), rows.end());
  for (const auto &row : rows) {
    mix_string(hash, row);
  }
  return hash_hex(hash);
}

[[nodiscard]] inline std::string compute_graph_first_channel_hash(
    const cuwacunu::ujcamei::source::contract::source_spec_t *source_spec) {
  if (!source_spec) {
    return "none";
  }
  std::uint64_t hash = kFnvOffsetBasis;
  mix_string(hash, "cuwacunu.ujcamei.source.retrieval.channels.v1");
  std::vector<std::string> rows;
  for (const auto &channel : source_spec->channel_forms) {
    std::ostringstream row;
    row << channel.active << "|"
        << cuwacunu::ujcamei::source::registry::types::enum_to_string(
               channel.interval)
        << "|" << channel.record_type << "|" << channel.input_length << "|"
        << channel.future_length << "|" << channel.channel_weight << "|"
        << channel.normalization_policy;
    rows.push_back(row.str());
  }
  std::sort(rows.begin(), rows.end());
  for (const auto &row : rows) {
    mix_string(hash, row);
  }
  return hash_hex(hash);
}

[[nodiscard]] inline std::string compute_graph_first_graph_hash(
    const cuwacunu::ujcamei::source::contract::source_spec_t *source_spec) {
  if (!source_spec) {
    return "none";
  }
  std::uint64_t hash = kFnvOffsetBasis;
  mix_string(hash, "cuwacunu.kikijyeba.topology.graph.v1");
  std::vector<std::string> rows;
  rows.reserve(source_spec->graph_node_forms.size() +
               source_spec->graph_edge_forms.size());
  for (const auto &node : source_spec->graph_node_forms) {
    std::ostringstream row;
    row << "graph_node|" << node.node_id << "|" << node.node_kind << "|"
        << node.active;
    rows.push_back(row.str());
  }
  for (const auto &edge : source_spec->graph_edge_forms) {
    std::ostringstream row;
    row << "graph_edge|" << edge.edge_id << "|" << edge.base_node << "|"
        << edge.quote_node << "|" << edge.source_instrument << "|"
        << edge.active;
    rows.push_back(row.str());
  }
  rows.push_back("edge_resolution_policy|" +
                 source_spec->graph_edge_resolution_policy);
  rows.push_back("edge_source_kind|" + source_spec->graph_edge_source_kind);
  std::sort(rows.begin(), rows.end());
  for (const auto &row : rows) {
    mix_string(hash, row);
  }
  return hash_hex(hash);
}

} // namespace nodelift_compatibility_detail

template <typename InstrumentMapT>
[[nodiscard]] inline nodelift_compatibility_report_t
validate_nodelift_compatibility(
    const cuwacunu::kikijyeba::topology::graph::market_graph_t &graph,
    const InstrumentMapT *edge_instruments,
    const cuwacunu::ujcamei::source::contract::source_spec_t *source_spec,
    const nodelift_compatibility_options_t &options = {}) {
  using nodelift_compatibility_detail::add_issue;

  nodelift_compatibility_report_t report{};
  report.node_count = static_cast<std::int64_t>(graph.node_ids.size());
  report.edge_count = static_cast<std::int64_t>(graph.edge_ids.size());

  if (options.expected_feature_width !=
      cuwacunu::ujcamei::source::registry::types::kKlineFeatureWidth) {
    add_issue(report, validation_severity_t::error,
              validation_code_t::feature_width_mismatch,
              "NodeLift v1 requires kline feature width 9");
  }

  std::unordered_set<std::string> seen_nodes;
  seen_nodes.reserve(graph.node_ids.size());
  for (const auto &node_id : graph.node_ids) {
    if (!seen_nodes.insert(node_id).second) {
      add_issue(report, validation_severity_t::error,
                validation_code_t::duplicate_node_id, "duplicate graph node id",
                std::nullopt, std::nullopt, node_id);
    }
  }

  const bool edge_shapes_match =
      graph.base_index.size() == graph.edge_ids.size() &&
      graph.quote_index.size() == graph.edge_ids.size();
  if (!edge_shapes_match) {
    add_issue(report, validation_severity_t::error,
              validation_code_t::invalid_node_index,
              "edge_ids/base_index/quote_index size mismatch");
  }

  std::unordered_set<std::string> seen_edges;
  seen_edges.reserve(graph.edge_ids.size());
  std::vector<std::int64_t> incident(graph.node_ids.size(), 0);
  std::map<std::pair<cuwacunu::kikijyeba::topology::graph::node_index_t,
                     cuwacunu::kikijyeba::topology::graph::node_index_t>,
           std::size_t>
      endpoint_to_edge;
  std::set<std::pair<std::size_t, std::size_t>> reverse_pairs;

  for (std::size_t e = 0; e < graph.edge_ids.size(); ++e) {
    const auto &edge_id = graph.edge_ids[e];
    if (!seen_edges.insert(edge_id).second) {
      add_issue(report, validation_severity_t::error,
                validation_code_t::duplicate_edge_id, "duplicate graph edge id",
                edge_id);
    }
    if (!edge_shapes_match) {
      continue;
    }
    const auto u = graph.base_index[e];
    const auto v = graph.quote_index[e];
    const bool valid_u =
        u >= 0 &&
        u < static_cast<cuwacunu::kikijyeba::topology::graph::node_index_t>(
                graph.node_ids.size());
    const bool valid_v =
        v >= 0 &&
        v < static_cast<cuwacunu::kikijyeba::topology::graph::node_index_t>(
                graph.node_ids.size());
    if (!valid_u || !valid_v) {
      add_issue(report, validation_severity_t::error,
                validation_code_t::invalid_node_index,
                "graph edge endpoint index outside node range", edge_id);
      continue;
    }
    if (u == v) {
      add_issue(report, validation_severity_t::error,
                validation_code_t::self_edge, "self edge is not valid",
                edge_id);
      continue;
    }
    ++incident[static_cast<std::size_t>(u)];
    ++incident[static_cast<std::size_t>(v)];

    const auto reverse = endpoint_to_edge.find(
        std::pair<cuwacunu::kikijyeba::topology::graph::node_index_t,
                  cuwacunu::kikijyeba::topology::graph::node_index_t>{v, u});
    if (reverse != endpoint_to_edge.end()) {
      reverse_pairs.insert(
          {std::min(e, reverse->second), std::max(e, reverse->second)});
    }
    endpoint_to_edge.emplace(
        std::pair<cuwacunu::kikijyeba::topology::graph::node_index_t,
                  cuwacunu::kikijyeba::topology::graph::node_index_t>{u, v},
        e);
  }

  if (options.warn_isolated_nodes) {
    for (std::size_t n = 0; n < graph.node_ids.size(); ++n) {
      if (incident[n] == 0) {
        ++report.isolated_node_count;
        add_issue(report, validation_severity_t::warning,
                  validation_code_t::isolated_node,
                  "graph node has no staged incident edge", std::nullopt,
                  std::nullopt, graph.node_ids[n]);
      }
    }
  }

  std::unordered_map<std::string, bool> source_proven_by_edge;
  source_proven_by_edge.reserve(graph.edge_ids.size());
  const bool validate_source =
      source_spec != nullptr && options.source_validation_mode ==
                                    source_validation_mode_t::construction_only;

  if (source_spec != nullptr && validate_source) {
    const auto active_channels =
        nodelift_compatibility_detail::active_channels(*source_spec);
    if (active_channels.empty()) {
      add_issue(report, validation_severity_t::error,
                validation_code_t::inactive_or_empty_channel_set,
                "ujcamei.source.retrieval.channels has no active channels");
    }

    bool active_channel_contract_ok = !active_channels.empty();
    for (const auto *channel : active_channels) {
      const auto label = nodelift_compatibility_detail::channel_label(*channel);
      if (options.require_kline_channels &&
          nodelift_compatibility_detail::trim_ascii(channel->record_type) !=
              "kline") {
        add_issue(report, validation_severity_t::error,
                  validation_code_t::non_kline_channel,
                  "NodeLift v1 requires active kline channels", std::nullopt,
                  std::nullopt, std::nullopt, label);
        active_channel_contract_ok = false;
      }
      std::int64_t input_length = 0;
      if (!nodelift_compatibility_detail::parse_positive_i64(
              channel->input_length, &input_length)) {
        add_issue(report, validation_severity_t::error,
                  validation_code_t::invalid_input_length,
                  "active channel input_length must be positive", std::nullopt,
                  std::nullopt, std::nullopt, label);
        active_channel_contract_ok = false;
      }
      if (options.include_future && options.require_future) {
        std::int64_t future_length = 0;
        if (!nodelift_compatibility_detail::parse_positive_i64(
                channel->future_length, &future_length)) {
          add_issue(report, validation_severity_t::error,
                    validation_code_t::invalid_future_length,
                    "required future channel length must be positive",
                    std::nullopt, std::nullopt, std::nullopt, label);
          active_channel_contract_ok = false;
        }
      }
      const std::string normalization =
          nodelift_compatibility_detail::trim_ascii(
              channel->normalization_policy)
                  .empty()
              ? std::string("none")
              : nodelift_compatibility_detail::trim_ascii(
                    channel->normalization_policy);
      if (options.require_log_returns_normalization &&
          normalization != "log_returns") {
        add_issue(report, validation_severity_t::error,
                  validation_code_t::normalization_policy_mismatch,
                  "NodeLift v1 requires log_returns-normalized kline "
                  "channels",
                  std::nullopt, std::nullopt, std::nullopt, label);
        active_channel_contract_ok = false;
      }
    }

    for (std::size_t e = 0; e < graph.edge_ids.size(); ++e) {
      const auto &edge_id = graph.edge_ids[e];
      source_proven_by_edge[edge_id] = false;
      if (!edge_instruments) {
        add_issue(
            report, validation_severity_t::error,
            validation_code_t::missing_source_row,
            "kikijyeba.topology.graph validation requested without edge "
            "instruments",
            edge_id);
        continue;
      }
      const auto instrument_found = edge_instruments->find(edge_id);
      if (instrument_found == edge_instruments->end()) {
        add_issue(report, validation_severity_t::error,
                  validation_code_t::missing_source_row,
                  "missing instrument signature for graph edge", edge_id);
        continue;
      }
      const auto &signature = instrument_found->second;
      std::string signature_error{};
      if (!cuwacunu::ujcamei::source::registry::instrument_signature_validate(
              signature, /*allow_any=*/false, "graph edge instrument",
              &signature_error)) {
        add_issue(report, validation_severity_t::error,
                  validation_code_t::missing_source_row,
                  "invalid graph edge instrument signature: " + signature_error,
                  edge_id);
        continue;
      }
      bool edge_sources_ok = active_channel_contract_ok;
      if (signature.symbol != edge_id) {
        add_issue(report, validation_severity_t::error,
                  validation_code_t::edge_id_symbol_mismatch,
                  "graph edge_id must equal instrument symbol", edge_id);
        edge_sources_ok = false;
      }
      if (edge_shapes_match && e < graph.base_index.size() &&
          e < graph.quote_index.size()) {
        const auto u = graph.base_index[e];
        const auto v = graph.quote_index[e];
        if (u >= 0 &&
            u < static_cast<
                    cuwacunu::kikijyeba::topology::graph::node_index_t>(
                    graph.node_ids.size()) &&
            v >= 0 &&
            v < static_cast<
                    cuwacunu::kikijyeba::topology::graph::node_index_t>(
                    graph.node_ids.size())) {
          const auto &base = graph.node_ids[static_cast<std::size_t>(u)];
          const auto &quote = graph.node_ids[static_cast<std::size_t>(v)];
          if (signature.base_asset != base || signature.quote_asset != quote) {
            add_issue(report, validation_severity_t::error,
                      validation_code_t::graph_source_base_quote_mismatch,
                      "graph endpoint assets do not match instrument "
                      "base/quote",
                      edge_id);
            edge_sources_ok = false;
          }
        }
      }

      bool edge_real_source_kind = true;
      for (const auto *channel : active_channels) {
        const auto label =
            nodelift_compatibility_detail::channel_label(*channel);
        const auto matches =
            source_spec->filter_source_forms(signature, channel->interval);
        if (matches.empty()) {
          add_issue(report, validation_severity_t::error,
                    validation_code_t::missing_source_row,
                    "missing exact Ujcamei source row for edge/channel",
                    edge_id, std::nullopt, std::nullopt, label);
          edge_sources_ok = false;
          continue;
        }
        if (options.require_unique_source_per_edge_interval &&
            matches.size() != 1) {
          add_issue(report, validation_severity_t::error,
                    validation_code_t::ambiguous_source_signature,
                    "expected exactly one source row for edge/channel", edge_id,
                    std::nullopt, std::nullopt, label);
          edge_sources_ok = false;
          continue;
        }
        if (nodelift_compatibility_detail::trim_ascii(matches.front().source)
                .empty()) {
          add_issue(report, validation_severity_t::error,
                    validation_code_t::missing_source_row,
                    "source row has empty source path", edge_id, std::nullopt,
                    std::nullopt, label);
          edge_sources_ok = false;
        }
        if (nodelift_compatibility_detail::trim_ascii(
                matches.front().source_kind) != "real") {
          edge_real_source_kind = false;
        }
      }
      source_proven_by_edge[edge_id] = edge_sources_ok && edge_real_source_kind;
    }
  }

  for (const auto &[lhs, rhs] : reverse_pairs) {
    ++report.reverse_pair_count;
    const auto &lhs_edge = graph.edge_ids[lhs];
    const auto &rhs_edge = graph.edge_ids[rhs];
    const bool proven =
        validate_source &&
        source_proven_by_edge.find(lhs_edge) != source_proven_by_edge.end() &&
        source_proven_by_edge.find(rhs_edge) != source_proven_by_edge.end() &&
        source_proven_by_edge.at(lhs_edge) &&
        source_proven_by_edge.at(rhs_edge);
    if (proven) {
      add_issue(report, validation_severity_t::warning,
                validation_code_t::reverse_pair_real,
                "reverse edge pair is allowed because both directions are "
                "proven real source instruments",
                lhs_edge, rhs_edge);
      continue;
    }
    if (options.reverse_edge_policy ==
        reverse_edge_policy_t::allow_without_proof_for_tests) {
      add_issue(report, validation_severity_t::warning,
                validation_code_t::reverse_pair_unproven,
                "reverse edge pair allowed without source proof by test "
                "policy",
                lhs_edge, rhs_edge);
      continue;
    }
    add_issue(report, validation_severity_t::error,
              validation_code_t::reverse_pair_unproven,
              "reverse edge pair cannot be used by NodeLift unless both "
              "directions are proven real source instruments",
              lhs_edge, rhs_edge);
  }

  return report;
}

[[nodiscard]] inline nodelift_compatibility_identity_t
make_nodelift_compatibility_identity(
    const cuwacunu::kikijyeba::topology::graph::market_graph_t &graph,
    const cuwacunu::ujcamei::source::contract::source_spec_t *source_spec) {
  return nodelift_compatibility_identity_t{
      .graph_fingerprint = graph.computed_graph_order_fingerprint(),
      .ujcamei_source_universe_hash =
          nodelift_compatibility_detail::compute_ujcamei_source_universe_hash(
              source_spec),
      .graph_first_channel_hash =
          nodelift_compatibility_detail::compute_graph_first_channel_hash(
              source_spec),
      .graph_first_graph_hash =
          nodelift_compatibility_detail::compute_graph_first_graph_hash(
              source_spec),
      .nodelift_version_token = "wikimyei.expression.nodelift.srl.v1",
  };
}

} // namespace cuwacunu::ujcamei::source::contract::validation
