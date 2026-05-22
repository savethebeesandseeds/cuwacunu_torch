/* contract.cpp */
#include "ujcamei/source/contract/contract.h"

#include <cctype>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace cuwacunu {
namespace ujcamei {
namespace source {
namespace contract {

namespace {

[[nodiscard]] std::string trim_ascii_ws_copy(std::string s) {
  std::size_t b = 0;
  while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b])))
    ++b;
  std::size_t e = s.size();
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1])))
    --e;
  return s.substr(b, e - b);
}

[[nodiscard]] bool is_true(std::string s) {
  return trim_ascii_ws_copy(std::move(s)) == "true";
}

} // namespace

std::vector<source_form_t> source_spec_t::filter_source_forms(
    const instrument_signature_t &target_signature,
    cuwacunu::ujcamei::source::registry::types::interval_type_e target_interval)
    const {
  return make_source_universe_from_compat(*this).filter_source_forms(
      target_signature, target_interval);
}

std::vector<source_form_t> source_universe_t::filter_source_forms(
    const instrument_signature_t &target_signature,
    cuwacunu::ujcamei::source::registry::types::interval_type_e target_interval)
    const {
  std::vector<source_form_t> result;
  for (const auto &form : source_forms) {
    if (form.interval != target_interval)
      continue;
    if (instrument_signature_compact_string(form.instrument_signature()) ==
        instrument_signature_compact_string(target_signature)) {
      result.push_back(form);
    }
  }
  return result;
}

bool source_universe_t::active_sources_match_runtime_signature(
    const instrument_signature_t &runtime_signature, std::string *error) const {
  if (error)
    error->clear();

  std::string validation_error{};
  if (!instrument_signature_validate(runtime_signature, /*allow_any=*/false,
                                     "RUNTIME_INSTRUMENT_SIGNATURE",
                                     &validation_error)) {
    if (error)
      *error = validation_error;
    return false;
  }

  for (const auto &form : source_forms) {
    if (instrument_signature_compact_string(form.instrument_signature()) ==
        instrument_signature_compact_string(runtime_signature)) {
      return true;
    }
  }

  if (error) {
    *error = "no source row for runtime instrument " +
             instrument_signature_compact_string(runtime_signature);
  }
  return false;
}

bool source_spec_t::active_sources_match_runtime_signature(
    const instrument_signature_t &runtime_signature, std::string *error) const {
  if (error)
    error->clear();

  std::string validation_error{};
  if (!instrument_signature_validate(runtime_signature, /*allow_any=*/false,
                                     "RUNTIME_INSTRUMENT_SIGNATURE",
                                     &validation_error)) {
    if (error)
      *error = validation_error;
    return false;
  }

  bool saw_active = false;
  for (const auto &channel : channel_forms) {
    if (trim_ascii_ws_copy(channel.active) != "true")
      continue;
    saw_active = true;
    if (trim_ascii_ws_copy(channel.record_type) !=
        runtime_signature.record_type) {
      if (error) {
        *error = "active source channel record_type '" +
                 trim_ascii_ws_copy(channel.record_type) +
                 "' does not match RUNTIME_INSTRUMENT_SIGNATURE.RECORD_TYPE " +
                 runtime_signature.record_type;
      }
      return false;
    }
    const auto matching_sources =
        filter_source_forms(runtime_signature, channel.interval);
    if (matching_sources.empty()) {
      if (error) {
        *error = "no source row row for runtime instrument " +
                 instrument_signature_compact_string(runtime_signature) +
                 " on an active interval";
      }
      return false;
    }
  }
  if (!saw_active) {
    if (error)
      *error = "source channel DSL has no active rows";
    return false;
  }
  return true;
}

std::vector<float> source_spec_t::retrieve_channel_weights() {
  std::vector<float> channel_weights;
  channel_weights.reserve(channel_forms.size());
  for (const auto &in_form : channel_forms) {
    if (in_form.active == "true") {
      try {
        channel_weights.push_back(
            static_cast<float>(std::stod(in_form.channel_weight)));
      } catch (...) {
        channel_weights.push_back(0.0f);
      }
    }
  }
  return channel_weights;
}

int64_t source_spec_t::count_channels() {
  int64_t count = 0;
  for (const auto &in_form : channel_forms) {
    if (in_form.active == "true")
      ++count;
  }
  return count;
}

int64_t source_spec_t::max_input_length() {
  int64_t max_input = 0;
  for (const auto &in_form : channel_forms) {
    if (in_form.active == "true") {
      try {
        const int64_t v =
            static_cast<int64_t>(std::stoll(in_form.input_length));
        if (v > max_input)
          max_input = v;
      } catch (...) {
      }
    }
  }
  return max_input;
}

int64_t source_spec_t::max_future_length() {
  int64_t max_future = 0;
  for (const auto &in_form : channel_forms) {
    if (in_form.active == "true") {
      try {
        const int64_t v =
            static_cast<int64_t>(std::stoll(in_form.future_length));
        if (v > max_future)
          max_future = v;
      } catch (...) {
      }
    }
  }
  return max_future;
}

cuwacunu::kikijyeba::topology::graph::market_graph_t
source_spec_t::active_market_graph() const {
  cuwacunu::kikijyeba::topology::graph::market_graph_t graph{};
  std::unordered_map<std::string,
                     cuwacunu::kikijyeba::topology::graph::node_index_t>
      node_to_index;
  node_to_index.reserve(graph_node_forms.size());

  for (const auto &node : graph_node_forms) {
    if (!is_true(node.active)) {
      continue;
    }
    const std::string node_id = trim_ascii_ws_copy(node.node_id);
    if (node_id.empty()) {
      throw std::runtime_error(
          "[source_spec_t] active graph node has empty id");
    }
    const auto index =
        static_cast<cuwacunu::kikijyeba::topology::graph::node_index_t>(
            graph.node_ids.size());
    if (!node_to_index.emplace(node_id, index).second) {
      throw std::runtime_error("[source_spec_t] duplicate active graph node: " +
                               node_id);
    }
    graph.node_ids.push_back(node_id);
  }

  for (const auto &edge : graph_edge_forms) {
    if (!is_true(edge.active)) {
      continue;
    }
    const std::string edge_id = trim_ascii_ws_copy(edge.edge_id);
    const std::string base_node = trim_ascii_ws_copy(edge.base_node);
    const std::string quote_node = trim_ascii_ws_copy(edge.quote_node);
    if (edge_id.empty() || base_node.empty() || quote_node.empty()) {
      throw std::runtime_error(
          "[source_spec_t] active graph edge has empty id or endpoint");
    }
    const auto base = node_to_index.find(base_node);
    const auto quote = node_to_index.find(quote_node);
    if (base == node_to_index.end() || quote == node_to_index.end()) {
      throw std::runtime_error("[source_spec_t] graph edge " + edge_id +
                               " references inactive or missing node");
    }
    graph.edge_ids.push_back(edge_id);
    graph.base_index.push_back(base->second);
    graph.quote_index.push_back(quote->second);
  }

  graph.validate();
  return graph;
}

std::unordered_map<std::string, instrument_signature_t>
source_spec_t::active_edge_instrument_map() const {
  std::unordered_map<std::string, instrument_signature_t> out;
  out.reserve(graph_edge_forms.size());

  for (const auto &edge : graph_edge_forms) {
    if (!is_true(edge.active)) {
      continue;
    }
    const std::string edge_id = trim_ascii_ws_copy(edge.edge_id);
    const std::string source_instrument =
        trim_ascii_ws_copy(edge.source_instrument);
    const std::string base_node = trim_ascii_ws_copy(edge.base_node);
    const std::string quote_node = trim_ascii_ws_copy(edge.quote_node);

    const source_form_t *matched = nullptr;
    for (const auto &source_form : source_forms) {
      if (trim_ascii_ws_copy(source_form.instrument) != source_instrument) {
        continue;
      }
      if (trim_ascii_ws_copy(source_form.base_asset) != base_node ||
          trim_ascii_ws_copy(source_form.quote_asset) != quote_node) {
        continue;
      }
      matched = &source_form;
      break;
    }
    if (!matched) {
      throw std::runtime_error("[source_spec_t] graph edge " + edge_id +
                               " has no matching source instrument row");
    }
    out.emplace(edge_id, matched->instrument_signature());
  }
  return out;
}

source_universe_t
make_source_universe_from_compat(const source_spec_t &compat) {
  source_universe_t out{};
  out.source_forms = compat.source_forms;
  out.csv_bootstrap_deltas = compat.csv_bootstrap_deltas;
  out.csv_step_abs_tol = compat.csv_step_abs_tol;
  out.csv_step_rel_tol = compat.csv_step_rel_tol;
  out.data_analytics_policy = compat.data_analytics_policy;
  return out;
}

source_spec_t
make_compat_source_spec_from_universe(const source_universe_t &universe) {
  source_spec_t out{};
  out.source_forms = universe.source_forms;
  out.csv_bootstrap_deltas = universe.csv_bootstrap_deltas;
  out.csv_step_abs_tol = universe.csv_step_abs_tol;
  out.csv_step_rel_tol = universe.csv_step_rel_tol;
  out.data_analytics_policy = universe.data_analytics_policy;
  return out;
}

} // namespace contract
} // namespace source
} // namespace ujcamei
} // namespace cuwacunu
