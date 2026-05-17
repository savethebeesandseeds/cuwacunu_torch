/* contract.h */
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "ujcamei/graph/graph.h"
#include "ujcamei/source/instrument_signature.h"
#include "ujcamei/source/types/types_enums.h"

namespace cuwacunu {
namespace ujcamei {
namespace source {
namespace contract {

using ::cuwacunu::ujcamei::source::instrument_signature_compact_string;
using ::cuwacunu::ujcamei::source::instrument_signature_t;
using ::cuwacunu::ujcamei::source::instrument_signature_validate;

/*
 * Stable source contract model.
 *
 * These structs describe the decoded source/dock compatibility bundle.
 * Ujcamei-owned source rows define what records exist. The channel and graph
 * rows are kikijyeba.protocol.cwu_01v dock settings owned semantically by
 * Kikijyeba composition and kept here only while lower storage and validation
 * surfaces still accept one merged shape. Runtime cursors are internal
 * dataloader/reporting selectors, not authored source contract rows. These
 * structs intentionally do not expose parser token hashes or runtime retrieval
 * state.
 */
struct source_form_t {
  std::string instrument;
  cuwacunu::ujcamei::source::types::interval_type_e interval;
  std::string record_type;
  std::string market_type;
  std::string venue;
  std::string base_asset;
  std::string quote_asset;
  std::string source_kind{"real"};
  std::string source;

  [[nodiscard]] instrument_signature_t instrument_signature() const {
    return instrument_signature_t{
        .symbol = instrument,
        .record_type = record_type,
        .market_type = market_type,
        .venue = venue,
        .base_asset = base_asset,
        .quote_asset = quote_asset,
    };
  }
};

struct channel_form_t {
  cuwacunu::ujcamei::source::types::interval_type_e interval;
  std::string active;
  std::string record_type;
  std::string input_length;
  std::string future_length;
  std::string channel_weight;
  std::string normalization_policy;
};

struct graph_node_form_t {
  std::string node_id;
  std::string node_kind;
  std::string active;
};

struct graph_edge_form_t {
  std::string edge_id;
  std::string base_node;
  std::string quote_node;
  std::string source_instrument;
  std::string active;
};

struct source_data_analytics_policy_t {
  bool declared{false};
  std::int64_t max_samples{0};
  std::int64_t max_features{0};
  long double mask_epsilon{0.0L};
  long double standardize_epsilon{0.0L};
};

struct source_universe_t {
  std::vector<source_form_t> source_forms;
  std::size_t csv_bootstrap_deltas{64};
  long double csv_step_abs_tol{1e-8L};
  long double csv_step_rel_tol{1e-10L};
  source_data_analytics_policy_t data_analytics_policy{};

  [[nodiscard]] bool empty() const { return source_forms.empty(); }

  std::vector<source_form_t> filter_source_forms(
      const instrument_signature_t &target_signature,
      cuwacunu::ujcamei::source::types::interval_type_e target_interval) const;
  bool active_sources_match_runtime_signature(
      const instrument_signature_t &runtime_signature,
      std::string *error = nullptr) const;
};

struct source_spec_t {
  std::vector<source_form_t> source_forms;
  std::vector<channel_form_t> channel_forms;
  std::vector<graph_node_form_t> graph_node_forms;
  std::vector<graph_edge_form_t> graph_edge_forms;
  std::string graph_edge_resolution_policy{"explicit_only"};
  std::string graph_edge_source_kind{"real"};
  std::string graph_fetch_mode{"serial"};
  std::string graph_max_fetch_workers{"0"};
  std::string graph_parallel_min_work_items{"16"};
  std::size_t csv_bootstrap_deltas{64};
  long double csv_step_abs_tol{1e-8L};
  long double csv_step_rel_tol{1e-10L};
  source_data_analytics_policy_t data_analytics_policy{};

  std::vector<source_form_t> filter_source_forms(
      const instrument_signature_t &target_signature,
      cuwacunu::ujcamei::source::types::interval_type_e target_interval) const;
  bool active_sources_match_runtime_signature(
      const instrument_signature_t &runtime_signature,
      std::string *error = nullptr) const;
  std::vector<float> retrieve_channel_weights();
  int64_t count_channels();
  int64_t max_input_length();
  int64_t max_future_length();
  cuwacunu::ujcamei::graph::market_graph_t active_market_graph() const;
  std::unordered_map<std::string, instrument_signature_t>
  active_edge_instrument_map() const;
};

[[nodiscard]] source_universe_t
make_source_universe_from_compat(const source_spec_t &compat);

[[nodiscard]] source_spec_t
make_compat_source_spec_from_universe(const source_universe_t &universe);

} /* namespace contract */
} /* namespace source */
} /* namespace ujcamei */
} /* namespace cuwacunu */
