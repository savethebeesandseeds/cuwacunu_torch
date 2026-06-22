/* runtime/decode.cpp */
#include "ujcamei/source/contract/runtime/decode.h"

#include "kikijyeba/topology/graph/graph_topology_decoder.h"
#include "kikijyeba/topology/graph/graph_topology_spec.h"
#include "ujcamei/source/registry/source_registry_decoder.h"
#include "ujcamei/source/retrieval/retrieval_channel_decoder.h"

#include "hero/config_derivation.h"
#include "hero/config_path_defaults.h"
#include "piaabo/core/utils.h"

#include <cctype>
#include <cerrno>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace cuwacunu {
namespace ujcamei {
namespace source {
namespace contract {

source_universe_t source_runtime_t::inst{};
source_runtime_t::_init source_runtime_t::_initializer{};

namespace {

std::string &source_runtime_last_config_path_() {
  static std::string path;
  return path;
}

[[nodiscard]] std::string trim_ascii_ws_copy(std::string s) {
  std::size_t b = 0;
  while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b])))
    ++b;
  std::size_t e = s.size();
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1])))
    --e;
  return s.substr(b, e - b);
}

[[nodiscard]] bool has_non_ws(const std::string &s) {
  for (const unsigned char c : s) {
    if (!std::isspace(c))
      return true;
  }
  return false;
}

[[nodiscard]] bool ends_with(std::string_view text, std::string_view suffix) {
  return text.size() >= suffix.size() &&
         text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

[[nodiscard]] std::string
resolve_config_relative_path(const std::string &config_path,
                             const std::string &raw_path) {
  std::filesystem::path path(trim_ascii_ws_copy(raw_path));
  if (path.empty() || path.is_absolute()) {
    return path.string();
  }
  return (std::filesystem::path(config_path).parent_path() / path)
      .lexically_normal()
      .string();
}

[[nodiscard]] bool valid_source_kind(const std::string &source_kind) {
  const std::string kind = trim_ascii_ws_copy(source_kind);
  return kind == "real" || kind == "synthetic" || kind == "derived";
}

void resolve_source_paths_relative_to_registry(
    source_universe_t &universe, const std::string &source_registry_dsl_path) {
  for (auto &source_form : universe.source_forms) {
    source_form.source = resolve_config_relative_path(source_registry_dsl_path,
                                                      source_form.source);
  }
}

void resolve_source_paths_relative_to_registry(
    source_spec_t &spec, const std::string &source_registry_dsl_path) {
  for (auto &source_form : spec.source_forms) {
    source_form.source = resolve_config_relative_path(source_registry_dsl_path,
                                                      source_form.source);
  }
}

[[nodiscard]] bool valid_bool_text(const std::string &text) {
  const std::string value = trim_ascii_ws_copy(text);
  return value == "true" || value == "false";
}

[[nodiscard]] bool valid_channel_record_type(const std::string &record_type) {
  const std::string value = trim_ascii_ws_copy(record_type);
  return value == "kline" || value == "trade" || value == "basic";
}

[[nodiscard]] bool
supported_runtime_record_type(const std::string &record_type) {
  return trim_ascii_ws_copy(record_type) == "kline";
}

[[nodiscard]] bool
valid_normalization_policy(const std::string &normalization_policy) {
  const std::string value = trim_ascii_ws_copy(normalization_policy);
  return value == "none" || value == "log_returns";
}

[[nodiscard]] bool valid_graph_edge_resolution_policy(const std::string &text) {
  const std::string value = trim_ascii_ws_copy(text);
  return value == "explicit_only" || value == "edge_discovery";
}

[[nodiscard]] bool valid_graph_fetch_mode(const std::string &text) {
  const std::string value = trim_ascii_ws_copy(text);
  return value == "serial" || value == "parallel_by_edge";
}

[[nodiscard]] bool is_true(const std::string &text) {
  return trim_ascii_ws_copy(text) == "true";
}

[[nodiscard]] std::string channel_key(
    cuwacunu::ujcamei::source::registry::types::interval_type_e interval,
    const std::string &record_type) {
  return cuwacunu::ujcamei::source::registry::types::enum_to_string(interval) +
         "/" + trim_ascii_ws_copy(record_type);
}

[[nodiscard]] std::int64_t
parse_positive_i64_field(const std::string &text, const std::string &field,
                         const std::string &row_label) {
  const std::string value = trim_ascii_ws_copy(text);
  if (value.empty()) {
    throw std::runtime_error(row_label + " has empty " + field);
  }

  std::int64_t parsed = 0;
  const char *begin = value.data();
  const char *end = value.data() + value.size();
  const auto result = std::from_chars(begin, end, parsed);
  if (result.ec != std::errc{} || result.ptr != end || parsed <= 0) {
    throw std::runtime_error(row_label + " has invalid " + field + " '" +
                             value + "'");
  }
  return parsed;
}

[[nodiscard]] std::int64_t
parse_nonnegative_i64_field(const std::string &text, const std::string &field,
                            const std::string &row_label) {
  const std::string value = trim_ascii_ws_copy(text);
  if (value.empty()) {
    throw std::runtime_error(row_label + " has empty " + field);
  }

  std::int64_t parsed = 0;
  const char *begin = value.data();
  const char *end = value.data() + value.size();
  const auto result = std::from_chars(begin, end, parsed);
  if (result.ec != std::errc{} || result.ptr != end || parsed < 0) {
    throw std::runtime_error(row_label + " has invalid " + field + " '" +
                             value + "'");
  }
  return parsed;
}

[[nodiscard]] double parse_finite_double_field(const std::string &text,
                                               const std::string &field,
                                               const std::string &row_label) {
  const std::string value = trim_ascii_ws_copy(text);
  if (value.empty()) {
    throw std::runtime_error(row_label + " has empty " + field);
  }

  errno = 0;
  char *end = nullptr;
  const double parsed = std::strtod(value.c_str(), &end);
  if (errno != 0 || end == nullptr || end != value.c_str() + value.size() ||
      !std::isfinite(parsed)) {
    throw std::runtime_error(row_label + " has invalid " + field + " '" +
                             value + "'");
  }
  return parsed;
}

[[nodiscard]] std::string read_text_file_or_throw(const std::string &path) {
  std::ifstream in(path);
  if (!in) {
    throw std::runtime_error("unable to open Ujcamei source config file: " +
                             path);
  }
  std::ostringstream oss;
  oss << in.rdbuf();
  return oss.str();
}

[[nodiscard]] std::unordered_map<std::string, std::string>
parse_simple_config_file(const std::string &config_path) {
  const std::string text = read_text_file_or_throw(config_path);
  std::unordered_map<std::string, std::string> out;
  std::istringstream lines(text);
  std::string line;
  std::size_t line_number = 0;
  while (std::getline(lines, line)) {
    ++line_number;
    const std::size_t comment = line.find('#');
    if (comment != std::string::npos) {
      line.resize(comment);
    }
    line = trim_ascii_ws_copy(std::move(line));
    if (line.empty()) {
      continue;
    }
    if (line.front() == '[' && line.back() == ']') {
      continue;
    }
    const std::size_t eq = line.find('=');
    if (eq == std::string::npos) {
      throw std::runtime_error("invalid config line " +
                               std::to_string(line_number) + " in " +
                               config_path + ": missing '='");
    }
    std::string key = trim_ascii_ws_copy(line.substr(0, eq));
    std::string value = trim_ascii_ws_copy(line.substr(eq + 1));
    if (key.empty() || value.empty()) {
      throw std::runtime_error("invalid config line " +
                               std::to_string(line_number) + " in " +
                               config_path + ": empty key or value");
    }
    if (ends_with(key, "_path")) {
      value = resolve_config_relative_path(config_path, value);
    }
    out[std::move(key)] = std::move(value);
  }
  return out;
}

[[nodiscard]] std::string
required_config_value(const std::unordered_map<std::string, std::string> &cfg,
                      const std::string &key, const std::string &config_path) {
  const auto it = cfg.find(key);
  if (it == cfg.end() || !has_non_ws(it->second)) {
    const auto derived =
        cuwacunu::hero::config_derivation::resolved_grammar_path_for_key(
            cfg, key, std::filesystem::path(config_path),
            true /* values_are_already_resolved */);
    if (derived.has_value() && !derived->empty()) {
      return derived->string();
    }
    throw std::runtime_error("missing required Ujcamei source config key '" +
                             key + "' in " + config_path);
  }
  return it->second;
}

[[nodiscard]] std::vector<const channel_form_t *>
active_channel_rows(const source_spec_t &spec) {
  std::vector<const channel_form_t *> out;
  out.reserve(spec.channel_forms.size());
  for (const auto &channel : spec.channel_forms) {
    if (is_true(channel.active)) {
      out.push_back(&channel);
    }
  }
  return out;
}

void validate_source_channel_contract_or_throw(const source_spec_t &spec) {
  if (spec.source_forms.empty()) {
    throw std::runtime_error(
        "sources DSL must declare at least one source row");
  }
  if (spec.channel_forms.empty()) {
    throw std::runtime_error("ujcamei.source.retrieval.channels DSL must "
                             "declare at least one channel row");
  }

  std::unordered_set<std::string> source_channels;
  source_channels.reserve(spec.source_forms.size());
  for (const auto &source_form : spec.source_forms) {
    source_channels.insert(
        channel_key(source_form.interval, source_form.record_type));
  }
  bool saw_active_channel = false;
  std::unordered_set<std::string> declared_channels;
  declared_channels.reserve(spec.channel_forms.size());
  for (const auto &channel : spec.channel_forms) {
    const std::string row_label =
        "ujcamei.source.retrieval.channels row " +
        channel_key(channel.interval, channel.record_type);

    if (!valid_bool_text(channel.active)) {
      throw std::runtime_error(row_label + " has invalid active value '" +
                               channel.active + "'");
    }
    saw_active_channel = saw_active_channel || is_true(channel.active);

    if (!valid_channel_record_type(channel.record_type)) {
      throw std::runtime_error(row_label + " has invalid record_type '" +
                               channel.record_type + "'");
    }
    if (is_true(channel.active) &&
        !supported_runtime_record_type(channel.record_type)) {
      throw std::runtime_error(
          row_label + " has active record_type '" + channel.record_type +
          "', but the current graph-first runtime supports only kline "
          "retrieval channels");
    }
    if (!declared_channels
             .insert(channel_key(channel.interval, channel.record_type))
             .second) {
      throw std::runtime_error("duplicate " + row_label);
    }

    (void)parse_positive_i64_field(channel.input_length, "input_length",
                                   row_label);
    (void)parse_positive_i64_field(channel.future_length, "future_length",
                                   row_label);
    const double channel_weight = parse_finite_double_field(
        channel.channel_weight, "channel_weight", row_label);
    if (std::abs(channel_weight - 1.0) > 1e-12) {
      throw std::runtime_error(
          row_label +
          " has channel_weight other than 1.0, but weighted channel "
          "contribution is not implemented in v1");
    }

    if (!valid_normalization_policy(channel.normalization_policy)) {
      throw std::runtime_error(row_label +
                               " has invalid normalization_policy '" +
                               channel.normalization_policy + "'");
    }

    if (source_channels.find(channel_key(
            channel.interval, channel.record_type)) == source_channels.end()) {
      throw std::runtime_error(row_label +
                               " has no matching interval/record_type in "
                               "Ujcamei sources DSL");
    }
  }

  if (!saw_active_channel) {
    throw std::runtime_error("ujcamei.source.retrieval.channels DSL must "
                             "declare at least one active channel row");
  }
}

void validate_graph_contract_or_throw(const source_spec_t &spec) {
  if (spec.graph_node_forms.empty() && spec.graph_edge_forms.empty()) {
    return;
  }
  const std::string edge_resolution_policy =
      trim_ascii_ws_copy(spec.graph_edge_resolution_policy);
  if (!valid_graph_edge_resolution_policy(edge_resolution_policy)) {
    throw std::runtime_error("kikijyeba.topology.graph DSL has invalid "
                             "EDGE_RESOLUTION_POLICY '" +
                             spec.graph_edge_resolution_policy + "'");
  }
  const std::string edge_source_kind =
      trim_ascii_ws_copy(spec.graph_edge_source_kind);
  if (!valid_source_kind(edge_source_kind)) {
    throw std::runtime_error(
        "kikijyeba.topology.graph DSL has invalid EDGE_SOURCE_KIND '" +
        spec.graph_edge_source_kind + "'");
  }
  const std::string fetch_mode = trim_ascii_ws_copy(spec.graph_fetch_mode);
  if (!valid_graph_fetch_mode(fetch_mode)) {
    throw std::runtime_error("kikijyeba.topology.graph DSL has invalid "
                             "FETCH_MODE '" +
                             spec.graph_fetch_mode + "'");
  }
  (void)parse_nonnegative_i64_field(spec.graph_max_fetch_workers,
                                    "MAX_FETCH_WORKERS",
                                    "kikijyeba.topology.graph GRAPH_POLICY");
  const auto parallel_min_work_items = parse_nonnegative_i64_field(
      spec.graph_parallel_min_work_items, "PARALLEL_MIN_WORK_ITEMS",
      "kikijyeba.topology.graph GRAPH_POLICY");
  if (parallel_min_work_items <= 0) {
    throw std::runtime_error("kikijyeba.topology.graph GRAPH_POLICY "
                             "PARALLEL_MIN_WORK_ITEMS must be positive");
  }

  std::unordered_set<std::string> active_nodes;
  active_nodes.reserve(spec.graph_node_forms.size());
  for (const auto &node : spec.graph_node_forms) {
    if (!valid_bool_text(node.active)) {
      throw std::runtime_error("graph node " + node.node_id +
                               " has invalid active value '" + node.active +
                               "'");
    }
    if (!is_true(node.active)) {
      continue;
    }
    const std::string node_id = trim_ascii_ws_copy(node.node_id);
    const std::string node_kind = trim_ascii_ws_copy(node.node_kind);
    if (node_id.empty()) {
      throw std::runtime_error("active graph node has empty node_id");
    }
    if (node_kind != "asset") {
      throw std::runtime_error("graph node " + node_id +
                               " has unsupported node_kind '" + node_kind +
                               "'");
    }
    if (!active_nodes.insert(node_id).second) {
      throw std::runtime_error("duplicate active graph node_id '" + node_id +
                               "'");
    }
  }

  if (active_nodes.empty()) {
    throw std::runtime_error(
        "kikijyeba.topology.graph DSL must declare at least one active "
        "node");
  }

  const auto active_channels = active_channel_rows(spec);
  if (active_channels.empty()) {
    throw std::runtime_error(
        "kikijyeba.topology.graph DSL requires at least one active "
        "channel");
  }

  std::unordered_set<std::string> active_edges;
  active_edges.reserve(spec.graph_edge_forms.size());
  for (const auto &edge : spec.graph_edge_forms) {
    if (!valid_bool_text(edge.active)) {
      throw std::runtime_error("graph edge " + edge.edge_id +
                               " has invalid active value '" + edge.active +
                               "'");
    }
    if (!is_true(edge.active)) {
      continue;
    }

    const std::string edge_id = trim_ascii_ws_copy(edge.edge_id);
    const std::string base_node = trim_ascii_ws_copy(edge.base_node);
    const std::string quote_node = trim_ascii_ws_copy(edge.quote_node);
    const std::string source_instrument =
        trim_ascii_ws_copy(edge.source_instrument);

    if (edge_id.empty() || base_node.empty() || quote_node.empty() ||
        source_instrument.empty()) {
      throw std::runtime_error(
          "active graph edge has empty id, endpoint, or source_instrument");
    }
    if (!active_edges.insert(edge_id).second) {
      throw std::runtime_error("duplicate active graph edge_id '" + edge_id +
                               "'");
    }
    if (edge_id != source_instrument) {
      throw std::runtime_error("graph edge " + edge_id +
                               " must use matching source_instrument in v1");
    }
    if (base_node == quote_node) {
      throw std::runtime_error("graph edge " + edge_id + " is a self edge");
    }
    if (active_nodes.find(base_node) == active_nodes.end() ||
        active_nodes.find(quote_node) == active_nodes.end()) {
      throw std::runtime_error("graph edge " + edge_id +
                               " references inactive or missing nodes");
    }

    for (const auto *channel : active_channels) {
      std::vector<const source_form_t *> matches;
      for (const auto &source_form : spec.source_forms) {
        if (source_form.interval != channel->interval) {
          continue;
        }
        if (trim_ascii_ws_copy(source_form.instrument) != source_instrument ||
            trim_ascii_ws_copy(source_form.record_type) !=
                trim_ascii_ws_copy(channel->record_type)) {
          continue;
        }
        matches.push_back(&source_form);
      }

      if (matches.empty()) {
        throw std::runtime_error(
            "kikijyeba.topology.graph graph edge " + edge_id +
            " has no Ujcamei source row for an active channel");
      }
      if (matches.size() != 1) {
        throw std::runtime_error(
            "kikijyeba.topology.graph graph edge " + edge_id +
            " has ambiguous Ujcamei source rows for an active channel");
      }
      const auto &source_form = *matches.front();
      if (trim_ascii_ws_copy(source_form.base_asset) != base_node ||
          trim_ascii_ws_copy(source_form.quote_asset) != quote_node) {
        throw std::runtime_error("graph edge " + edge_id +
                                 " endpoint nodes do not match source row "
                                 "base/quote");
      }
      if (trim_ascii_ws_copy(source_form.source_kind) != edge_source_kind) {
        throw std::runtime_error("graph edge " + edge_id +
                                 " must resolve to source rows with "
                                 "EDGE_SOURCE_KIND '" +
                                 edge_source_kind + "'");
      }
    }
  }

  if (active_edges.empty()) {
    if (edge_resolution_policy == "edge_discovery") {
      return;
    }
    throw std::runtime_error(
        "kikijyeba.topology.graph DSL must declare at least one active "
        "edge");
  }

  (void)spec.active_market_graph();
}

} // namespace

void source_runtime_t::init() {
  log_dbg("[source_runtime_t] initializing static-global source snapshot "
          "(single mutable cache updated by explicit runtime call)\n");
}

void source_runtime_t::finit() {
  const auto &last_config_path = source_runtime_last_config_path_();
  const char *last_path =
      last_config_path.empty() ? "<none>" : last_config_path.c_str();
  log_dbg("[source_runtime_t] finalizing static-global source snapshot "
          "(last_config_path=%s)\n",
          last_path);
}

void source_runtime_t::update_from_config(std::string config_path) {
  config_path = trim_ascii_ws_copy(std::move(config_path));
  const std::string resolved_path =
      has_non_ws(config_path) ? config_path : default_source_config_path();
  inst = decode_source_universe_from_config(resolved_path);
  source_runtime_last_config_path_() = resolved_path;
}

void validate_source_universe_or_throw(const source_universe_t &universe) {
  if (universe.source_forms.empty()) {
    throw std::runtime_error(
        "sources DSL must declare at least one source row");
  }
  if (!universe.data_analytics_policy.declared) {
    throw std::runtime_error(
        "DATA_ANALYTICS_POLICY block is required in sources DSL");
  }
  if (universe.data_analytics_policy.max_samples <= 0 ||
      universe.data_analytics_policy.max_features <= 0) {
    throw std::runtime_error(
        "DATA_ANALYTICS_POLICY requires MAX_SAMPLES and MAX_FEATURES > 0");
  }
  if (universe.data_analytics_policy.mask_epsilon < 0.0L) {
    throw std::runtime_error("DATA_ANALYTICS_POLICY.MASK_EPSILON must be >= 0");
  }
  if (!(universe.data_analytics_policy.standardize_epsilon > 0.0L)) {
    throw std::runtime_error(
        "DATA_ANALYTICS_POLICY.STANDARDIZE_EPSILON must be > 0");
  }
  for (const auto &source_form : universe.source_forms) {
    std::string signature_error{};
    if (!instrument_signature_validate(
            source_form.instrument_signature(), /*allow_any=*/false,
            "source row " + source_form.instrument, &signature_error)) {
      throw std::runtime_error(signature_error);
    }
    if (!valid_source_kind(source_form.source_kind)) {
      throw std::runtime_error("source row " + source_form.instrument +
                               " has invalid source_kind '" +
                               source_form.source_kind + "'");
    }
  }
}

source_universe_t
decode_source_universe_from_split_dsl(std::string source_grammar,
                                      std::string source_instruction) {
  if (!has_non_ws(source_grammar) || !has_non_ws(source_instruction)) {
    throw std::runtime_error("Ujcamei sources DSL is required");
  }
  cuwacunu::ujcamei::source::registry::source_registry_decoder_t
      sources_decoder(std::move(source_grammar));
  source_spec_t sources_part = sources_decoder.decode(source_instruction);
  auto universe = make_source_universe_from_compat(sources_part);
  validate_source_universe_or_throw(universe);
  return universe;
}

source_spec_t decode_source_spec_from_split_dsl(std::string source_grammar,
                                                std::string source_instruction,
                                                std::string channel_grammar,
                                                std::string channel_instruction,
                                                std::string graph_grammar,
                                                std::string graph_instruction) {
  if (has_non_ws(source_grammar) && has_non_ws(source_instruction) &&
      has_non_ws(channel_grammar) && has_non_ws(channel_instruction)) {
    cuwacunu::ujcamei::source::retrieval::retrieval_channel_decoder_t
        channels_decoder(std::move(channel_grammar));

    source_universe_t source_universe = decode_source_universe_from_split_dsl(
        std::move(source_grammar), std::move(source_instruction));
    source_spec_t channels_part = channels_decoder.decode(channel_instruction);
    cuwacunu::kikijyeba::topology::graph::graph_topology_spec_t graph_part{};
    if (has_non_ws(graph_grammar) || has_non_ws(graph_instruction)) {
      if (!has_non_ws(graph_grammar) || !has_non_ws(graph_instruction)) {
        throw std::runtime_error(
            "graph DSL requires both grammar and instruction text");
      }
      cuwacunu::kikijyeba::topology::graph::graph_topology_decoder_t
          graph_decoder(std::move(graph_grammar));
      graph_part = graph_decoder.decode(graph_instruction);
    }

    source_spec_t merged{};
    merged.source_forms = std::move(source_universe.source_forms);
    merged.channel_forms = std::move(channels_part.channel_forms);
    merged.graph_node_forms = std::move(graph_part.graph_node_forms);
    merged.graph_edge_forms = std::move(graph_part.graph_edge_forms);
    merged.graph_edge_resolution_policy =
        std::move(graph_part.edge_resolution_policy);
    merged.graph_edge_source_kind = std::move(graph_part.edge_source_kind);
    merged.graph_fetch_mode = std::move(graph_part.fetch_mode);
    merged.graph_max_fetch_workers = std::move(graph_part.max_fetch_workers);
    merged.graph_parallel_min_work_items =
        std::move(graph_part.parallel_min_work_items);
    merged.csv_bootstrap_deltas = source_universe.csv_bootstrap_deltas;
    merged.csv_step_abs_tol = source_universe.csv_step_abs_tol;
    merged.csv_step_rel_tol = source_universe.csv_step_rel_tol;
    merged.data_analytics_policy = source_universe.data_analytics_policy;
    validate_source_channel_contract_or_throw(merged);
    validate_graph_contract_or_throw(merged);
    return merged;
  }

  throw std::runtime_error("split source DSL is required");
}

std::string default_source_config_path() {
  return cuwacunu::hero::config_paths::default_global_config_path().string();
}

source_registry_config_paths_t
load_source_registry_config_paths_from_config(std::string config_path) {
  config_path = trim_ascii_ws_copy(std::move(config_path));
  if (!has_non_ws(config_path)) {
    config_path = default_source_config_path();
  }
  const auto cfg = parse_simple_config_file(config_path);
  return source_registry_config_paths_t{
      .source_registry_dsl_bnf_path = required_config_value(
          cfg, "ujcamei_source_registry_dsl_bnf_path", config_path),
      .source_registry_dsl_path = required_config_value(
          cfg, "ujcamei_source_registry_dsl_path", config_path),
  };
}

source_spec_t decode_source_spec_from_config(std::string config_path) {
  config_path = trim_ascii_ws_copy(std::move(config_path));
  if (!has_non_ws(config_path)) {
    config_path = default_source_config_path();
  }
  const auto paths = load_source_registry_config_paths_from_config(config_path);
  const auto cfg = parse_simple_config_file(config_path);
  const auto graph_first_retrieval_channels_dsl_bnf_path =
      required_config_value(
          cfg, "ujcamei_source_retrieval_channels_dsl_bnf_path", config_path);
  const auto graph_first_retrieval_channels_dsl_path = required_config_value(
      cfg, "ujcamei_source_retrieval_channels_dsl_path", config_path);
  const auto graph_first_graph_dsl_bnf_path = required_config_value(
      cfg, "kikijyeba_topology_graph_dsl_bnf_path", config_path);
  const auto graph_first_graph_dsl_path = required_config_value(
      cfg, "kikijyeba_topology_graph_dsl_path", config_path);
  auto spec = decode_source_spec_from_split_dsl(
      read_text_file_or_throw(paths.source_registry_dsl_bnf_path),
      read_text_file_or_throw(paths.source_registry_dsl_path),
      read_text_file_or_throw(graph_first_retrieval_channels_dsl_bnf_path),
      read_text_file_or_throw(graph_first_retrieval_channels_dsl_path),
      read_text_file_or_throw(graph_first_graph_dsl_bnf_path),
      read_text_file_or_throw(graph_first_graph_dsl_path));
  resolve_source_paths_relative_to_registry(spec,
                                            paths.source_registry_dsl_path);
  return spec;
}

source_universe_t decode_source_universe_from_config(std::string config_path) {
  const auto paths =
      load_source_registry_config_paths_from_config(std::move(config_path));
  auto universe = decode_source_universe_from_split_dsl(
      read_text_file_or_throw(paths.source_registry_dsl_bnf_path),
      read_text_file_or_throw(paths.source_registry_dsl_path));
  resolve_source_paths_relative_to_registry(universe,
                                            paths.source_registry_dsl_path);
  return universe;
}

source_universe_t decode_source_universe_from_default_config() {
  return decode_source_universe_from_config(default_source_config_path());
}

source_spec_t decode_source_spec_from_default_config() {
  return decode_source_spec_from_config(default_source_config_path());
}

} // namespace contract
} // namespace source
} // namespace ujcamei
} // namespace cuwacunu
