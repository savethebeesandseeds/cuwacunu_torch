// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cstddef>
#include <exception>
#include <future>
#include <iterator>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <torch/torch.h>

#include "ujcamei/graph/graph.h"
#include "ujcamei/source/contract/contract.h"
#include "ujcamei/source/contract/validation/nodelift_compatibility.h"
#include "ujcamei/source/dataloader/graph_anchor_edge_batch.h"
#include "ujcamei/source/instrument_signature.h"
#include "ujcamei/source/storage/memory_mapped/memory_mapped_dataset.h"
#include "ujcamei/source/types/kline_feature_registry.h"

namespace cuwacunu::ujcamei::source::dataloader {

enum class fetch_mode_t {
  serial,
  parallel_by_edge,
};

using cuwacunu::ujcamei::source::contract::validation::
    make_nodelift_compatibility_identity;
using cuwacunu::ujcamei::source::contract::validation::
    nodelift_compatibility_identity_t;
using cuwacunu::ujcamei::source::contract::validation::
    nodelift_compatibility_options_t;
using cuwacunu::ujcamei::source::contract::validation::
    nodelift_compatibility_report_t;
using cuwacunu::ujcamei::source::contract::validation::reverse_edge_policy_t;
using cuwacunu::ujcamei::source::contract::validation::source_validation_mode_t;
using cuwacunu::ujcamei::source::contract::validation::
    validate_nodelift_compatibility;
using cuwacunu::ujcamei::source::contract::validation::validation_code_t;
using cuwacunu::ujcamei::source::contract::validation::validation_severity_t;

template <typename DatatypeT> struct graph_anchor_edge_dataset_options_t {
  using key_t = typename DatatypeT::key_type_t;

  std::optional<cuwacunu::ujcamei::graph::instrument_edge_id_t>
      reference_edge_id{std::nullopt};

  bool force_rebuild_cache{false};
  bool require_all_edges{true};
  bool require_normalized{true};
  bool include_future{true};
  bool require_future{true};
  bool include_keys{true};
  bool validate_keys_against_anchor{true};
  bool validate_anchor_fetch{true};
  int64_t feature_width{cuwacunu::ujcamei::source::types::kKlineFeatureWidth};
  graph_anchor_order_policy_t anchor_order{graph_anchor_order_policy_t::sorted};
  nodelift_compatibility_options_t validation_options{};
  fetch_mode_t fetch_mode{fetch_mode_t::serial};
  int64_t max_fetch_workers{0};
  int64_t parallel_min_work_items{16};
};

template <typename DatatypeT> class graph_anchor_edge_dataset_t {
  static_assert(std::is_integral_v<typename DatatypeT::key_type_t>,
                "graph_anchor_edge_dataset_t v1 requires integral key_type_t");

public:
  using key_t = typename DatatypeT::key_type_t;
  using edge_dataset_t = cuwacunu::ujcamei::source::storage::memory_mapped::
      MemoryMappedEdgeDataset<DatatypeT>;
  using edge_dataset_map_t =
      std::unordered_map<cuwacunu::ujcamei::graph::instrument_edge_id_t,
                         edge_dataset_t>;
  using instrument_map_t =
      std::unordered_map<cuwacunu::ujcamei::graph::instrument_edge_id_t,
                         cuwacunu::ujcamei::source::instrument_signature_t>;
  using source_materialization_request_t = cuwacunu::ujcamei::source::storage::
      memory_mapped::source_materialization_request_t;
  using anchor_sample_t = graph_anchor_edge_sample_t<key_t>;
  using graph_batch_t = graph_anchor_edge_batch_t<key_t>;

  struct source_plan_t {
    cuwacunu::ujcamei::graph::market_graph_t graph{};
    instrument_map_t edge_instruments{};
    source_materialization_request_t materialization_request{};
    std::optional<cuwacunu::ujcamei::source::contract::source_spec_t>
        validation_source_spec{std::nullopt};
  };

  graph_anchor_edge_dataset_t() = default;
  graph_anchor_edge_dataset_t(const graph_anchor_edge_dataset_t &) = delete;
  graph_anchor_edge_dataset_t &
  operator=(const graph_anchor_edge_dataset_t &) = delete;
  graph_anchor_edge_dataset_t(graph_anchor_edge_dataset_t &&) = default;
  graph_anchor_edge_dataset_t &
  operator=(graph_anchor_edge_dataset_t &&) = default;

  graph_anchor_edge_dataset_t(
      cuwacunu::ujcamei::graph::market_graph_t graph,
      edge_dataset_map_t edge_datasets,
      graph_anchor_edge_dataset_options_t<DatatypeT> options = {})
      : graph_(std::move(graph)), edge_datasets_(std::move(edge_datasets)),
        options_(std::move(options)) {
    run_validation_(nullptr, nullptr);
    initialize_after_validation_();
  }

  graph_anchor_edge_dataset_t(
      cuwacunu::ujcamei::source::contract::source_spec_t source_spec,
      graph_anchor_edge_dataset_options_t<DatatypeT> options = {})
      : graph_anchor_edge_dataset_t(
            make_source_spec_plan_(std::move(source_spec)),
            std::move(options)) {}

  graph_anchor_edge_dataset_t(
      cuwacunu::ujcamei::graph::market_graph_t graph,
      instrument_map_t edge_instruments,
      cuwacunu::ujcamei::source::contract::source_spec_t source_spec,
      graph_anchor_edge_dataset_options_t<DatatypeT> options = {})
      : graph_anchor_edge_dataset_t(
            make_source_spec_plan_(std::move(graph),
                                   std::move(edge_instruments),
                                   std::move(source_spec)),
            std::move(options)) {}

  graph_anchor_edge_dataset_t(
      source_plan_t source_plan,
      graph_anchor_edge_dataset_options_t<DatatypeT> options = {})
      : graph_(std::move(source_plan.graph)), options_(std::move(options)) {
    const auto *validation_source_spec =
        source_plan.validation_source_spec.has_value()
            ? &source_plan.validation_source_spec.value()
            : nullptr;
    run_validation_(&source_plan.edge_instruments, validation_source_spec);
    validate_options_();
    for (const auto &edge_id : graph_.edge_ids) {
      const auto found = source_plan.edge_instruments.find(edge_id);
      TORCH_CHECK(
          found != source_plan.edge_instruments.end(),
          "[graph_anchor_edge_dataset_t] missing instrument for edge_id=",
          edge_id);
      edge_datasets_.emplace(
          edge_id, cuwacunu::ujcamei::source::storage::memory_mapped::
                       create_memory_mapped_edge_dataset<DatatypeT>(
                           found->second, source_plan.materialization_request,
                           options_.force_rebuild_cache));
    }
    validate_edge_datasets_();
    build_anchor_keys_();
  }

  [[nodiscard]] std::size_t size() const { return anchor_keys_.size(); }

  [[nodiscard]] bool empty() const { return anchor_keys_.empty(); }

  [[nodiscard]] const std::vector<key_t> &anchor_keys() const {
    return anchor_keys_;
  }

  [[nodiscard]] const std::vector<
      cuwacunu::ujcamei::graph::instrument_edge_id_t> &
  edge_ids() const {
    return graph_.edge_ids;
  }

  [[nodiscard]] const cuwacunu::ujcamei::graph::market_graph_t &
  market_graph() const {
    return graph_;
  }

  [[nodiscard]] const graph_anchor_edge_dataset_options_t<DatatypeT> &
  options() const {
    return options_;
  }

  [[nodiscard]] const nodelift_compatibility_report_t &
  validation_report() const {
    return validation_report_;
  }

  [[nodiscard]] const nodelift_compatibility_identity_t &
  validation_identity() const {
    return validation_identity_;
  }

  [[nodiscard]] const std::string &graph_order_fingerprint() const {
    return graph_order_fingerprint_;
  }

  [[nodiscard]] const graph_anchor_cursor_report_t<key_t> &
  cursor_report() const {
    return cursor_report_;
  }

  [[nodiscard]] graph_anchor_edge_batch_options_t
  graph_anchor_edge_batch_options() const {
    graph_anchor_edge_batch_options_t out{};
    out.edge_ids = graph_.edge_ids;
    out.require_all_edges = options_.require_all_edges;
    out.require_normalized = options_.require_normalized;
    out.include_future = options_.include_future;
    out.require_future = options_.require_future;
    out.future_length = std::nullopt;
    out.include_keys = options_.include_keys;
    out.validate_keys_against_anchor = options_.validate_keys_against_anchor;
    out.feature_width = options_.feature_width;
    out.anchor_order = options_.anchor_order;
    return out;
  }

  [[nodiscard]] std::vector<anchor_sample_t>
  get_edge_samples(std::size_t anchor_index) {
    TORCH_CHECK(anchor_index < anchor_keys_.size(),
                "[graph_anchor_edge_dataset_t] anchor_index out of range");
    return get_edge_samples_by_anchor_key(anchor_keys_[anchor_index]);
  }

  [[nodiscard]] std::vector<anchor_sample_t>
  get_edge_samples_by_anchor_key(key_t anchor_key) {
    std::vector<anchor_sample_t> out;
    out.reserve(graph_.edge_ids.size());

    for (const auto &edge_id : graph_.edge_ids) {
      auto found = edge_datasets_.find(edge_id);
      if (found == edge_datasets_.end()) {
        TORCH_CHECK(!options_.require_all_edges,
                    "[graph_anchor_edge_dataset_t] missing edge dataset for "
                    "edge_id=",
                    edge_id);
        continue;
      }

      try {
        auto sample = found->second.get_by_anchor_key(anchor_key);
        sample.edge_id = edge_id;
        out.push_back(anchor_sample_t{
            .edge_id = edge_id,
            .anchor_key = anchor_key,
            .sample = std::move(sample),
        });
      } catch (const std::exception &ex) {
        TORCH_CHECK(
            !options_.require_all_edges,
            "[graph_anchor_edge_dataset_t] failed to fetch edge_id=", edge_id,
            " at anchor_key=", anchor_key, " error=", ex.what());
      }
    }
    return out;
  }

  [[nodiscard]] graph_batch_t get_graph_batch(
      std::size_t begin_anchor_index, std::size_t batch_size,
      std::optional<graph_anchor_edge_batch_options_t> collator_options =
          std::nullopt) {
    TORCH_CHECK(batch_size > 0,
                "[graph_anchor_edge_dataset_t] batch_size must be positive");
    TORCH_CHECK(
        begin_anchor_index < anchor_keys_.size(),
        "[graph_anchor_edge_dataset_t] begin_anchor_index out of range");

    const std::size_t end_anchor_index =
        std::min(anchor_keys_.size(), begin_anchor_index + batch_size);
    auto samples =
        collect_samples_for_anchor_range_(begin_anchor_index, end_anchor_index);

    auto options = collator_options.value_or(graph_anchor_edge_batch_options());
    if (options.edge_ids.empty()) {
      options.edge_ids = graph_.edge_ids;
    }
    TORCH_CHECK(
        options.edge_ids == graph_.edge_ids,
        "[graph_anchor_edge_dataset_t] collator edge_ids must match graph "
        "edge order");
    auto batch = make_graph_anchor_edge_batch<key_t>(samples, options);
    batch.graph_order_fingerprint = graph_order_fingerprint_;
    std::vector<key_t> cursor_anchors;
    cursor_anchors.reserve(end_anchor_index - begin_anchor_index);
    for (std::size_t i = begin_anchor_index; i < end_anchor_index; ++i) {
      cursor_anchors.push_back(anchor_keys_[i]);
    }
    batch.cursor = make_graph_anchor_cursor<key_t>(
        graph_order_fingerprint_, begin_anchor_index, end_anchor_index,
        batch_size, std::move(cursor_anchors));
    return batch;
  }

private:
  [[nodiscard]] static source_plan_t make_source_spec_plan_(
      cuwacunu::ujcamei::source::contract::source_spec_t source_spec) {
    source_plan_t out{};
    out.graph = source_spec.active_market_graph();
    out.edge_instruments = source_spec.active_edge_instrument_map();
    out.materialization_request = cuwacunu::ujcamei::source::storage::
        memory_mapped::make_source_materialization_request(source_spec);
    out.validation_source_spec = std::move(source_spec);
    return out;
  }

  [[nodiscard]] static source_plan_t make_source_spec_plan_(
      cuwacunu::ujcamei::graph::market_graph_t graph,
      instrument_map_t edge_instruments,
      cuwacunu::ujcamei::source::contract::source_spec_t source_spec) {
    source_plan_t out{};
    out.graph = std::move(graph);
    out.edge_instruments = std::move(edge_instruments);
    out.materialization_request = cuwacunu::ujcamei::source::storage::
        memory_mapped::make_source_materialization_request(source_spec);
    out.validation_source_spec = std::move(source_spec);
    return out;
  }

  void initialize_after_validation_() {
    validate_options_();
    validate_edge_datasets_();
    build_anchor_keys_();
  }

  [[nodiscard]] nodelift_compatibility_options_t
  effective_validation_options_() const {
    auto out = options_.validation_options;
    out.expected_feature_width = options_.feature_width;
    out.include_future = options_.include_future;
    out.require_future = options_.require_future;
    return out;
  }

  void run_validation_(
      const instrument_map_t *edge_instruments,
      const cuwacunu::ujcamei::source::contract::source_spec_t *source_spec) {
    graph_order_fingerprint_ = graph_.computed_graph_order_fingerprint();
    validation_identity_ =
        make_nodelift_compatibility_identity(graph_, source_spec);
    validation_report_ = validate_nodelift_compatibility(
        graph_, edge_instruments, source_spec, effective_validation_options_());
    TORCH_CHECK(
        !validation_report_.has_errors(),
        "[graph_anchor_edge_dataset_t] graph/source validation failed:\n",
        validation_report_.summary());
    graph_.validate();
  }

  void validate_options_() const {
    TORCH_CHECK(options_.feature_width ==
                    cuwacunu::ujcamei::source::types::kKlineFeatureWidth,
                "[graph_anchor_edge_dataset_t] v1 requires feature_width=9");
    if (options_.reference_edge_id.has_value()) {
      const auto &reference = *options_.reference_edge_id;
      TORCH_CHECK(
          std::find(graph_.edge_ids.begin(), graph_.edge_ids.end(),
                    reference) != graph_.edge_ids.end(),
          "[graph_anchor_edge_dataset_t] reference_edge_id not present in "
          "graph edge_ids: ",
          reference);
    }
  }

  void validate_edge_datasets_() {
    TORCH_CHECK(
        !graph_.edge_ids.empty(),
        "[graph_anchor_edge_dataset_t] graph edge_ids must not be empty");
    for (const auto &edge_id : graph_.edge_ids) {
      const auto found = edge_datasets_.find(edge_id);
      if (found == edge_datasets_.end()) {
        cuwacunu::ujcamei::source::contract::validation::
            nodelift_compatibility_detail::add_issue(
                validation_report_, validation_severity_t::error,
                validation_code_t::missing_edge_dataset,
                "missing edge-local dataset for staged graph edge", edge_id);
        TORCH_CHECK(false,
                    "[graph_anchor_edge_dataset_t] missing edge dataset for "
                    "edge_id=",
                    edge_id);
      }
      if (found->second.num_records_ == 0) {
        cuwacunu::ujcamei::source::contract::validation::
            nodelift_compatibility_detail::add_issue(
                validation_report_, validation_severity_t::error,
                validation_code_t::missing_edge_dataset,
                "empty edge-local dataset for staged graph edge", edge_id);
        TORCH_CHECK(
            false,
            "[graph_anchor_edge_dataset_t] empty edge dataset for edge_id=",
            edge_id);
      }
    }
  }

  [[nodiscard]] const edge_dataset_t &reference_dataset_() const {
    const auto &reference_edge_id =
        options_.reference_edge_id.value_or(graph_.edge_ids.front());
    const auto found = edge_datasets_.find(reference_edge_id);
    TORCH_CHECK(
        found != edge_datasets_.end(),
        "[graph_anchor_edge_dataset_t] reference edge dataset missing: ",
        reference_edge_id);
    return found->second;
  }

  [[nodiscard]] bool edge_dataset_covers_anchor_(const edge_dataset_t &dataset,
                                                 key_t anchor_key) const {
    return dataset.can_get_by_anchor_key(anchor_key);
  }

  [[nodiscard]] bool all_edges_cover_anchor_(key_t anchor_key) const {
    for (const auto &edge_id : graph_.edge_ids) {
      const auto found = edge_datasets_.find(edge_id);
      if (found == edge_datasets_.end() ||
          !edge_dataset_covers_anchor_(found->second, anchor_key)) {
        return false;
      }
    }
    return true;
  }

  [[nodiscard]] bool all_edges_fetch_anchor_(key_t anchor_key) {
    if (!options_.validate_anchor_fetch) {
      return true;
    }
    for (const auto &edge_id : graph_.edge_ids) {
      auto found = edge_datasets_.find(edge_id);
      std::string reason{};
      if (found == edge_datasets_.end() ||
          !found->second.can_get_by_anchor_key_strict(anchor_key, &reason)) {
        return false;
      }
    }
    return true;
  }

  [[nodiscard]] std::vector<anchor_sample_t>
  collect_samples_for_anchor_range_(std::size_t begin_anchor_index,
                                    std::size_t end_anchor_index) {
    if (options_.fetch_mode == fetch_mode_t::parallel_by_edge) {
      const std::size_t work_items =
          (end_anchor_index - begin_anchor_index) * graph_.edge_ids.size();
      if (work_items >= static_cast<std::size_t>(std::max<int64_t>(
                            1, options_.parallel_min_work_items))) {
        return collect_samples_parallel_by_edge_(begin_anchor_index,
                                                 end_anchor_index);
      }
    }
    std::vector<anchor_sample_t> samples;
    samples.reserve((end_anchor_index - begin_anchor_index) *
                    graph_.edge_ids.size());
    for (std::size_t i = begin_anchor_index; i < end_anchor_index; ++i) {
      auto edge_samples = get_edge_samples(i);
      samples.insert(samples.end(),
                     std::make_move_iterator(edge_samples.begin()),
                     std::make_move_iterator(edge_samples.end()));
    }
    return samples;
  }

  [[nodiscard]] std::vector<anchor_sample_t>
  collect_samples_parallel_by_edge_(std::size_t begin_anchor_index,
                                    std::size_t end_anchor_index) {
    const std::size_t B = end_anchor_index - begin_anchor_index;
    const std::size_t L = graph_.edge_ids.size();

    std::vector<key_t> anchors;
    anchors.reserve(B);
    for (std::size_t i = begin_anchor_index; i < end_anchor_index; ++i) {
      anchors.push_back(anchor_keys_[i]);
    }

    std::vector<std::pair<cuwacunu::ujcamei::graph::instrument_edge_id_t,
                          edge_dataset_t *>>
        edge_refs;
    edge_refs.reserve(L);
    for (const auto &edge_id : graph_.edge_ids) {
      auto found = edge_datasets_.find(edge_id);
      TORCH_CHECK(
          found != edge_datasets_.end(),
          "[graph_anchor_edge_dataset_t] missing edge dataset for edge_id=",
          edge_id);
      edge_refs.emplace_back(edge_id, &found->second);
    }

    std::vector<std::vector<std::optional<anchor_sample_t>>> by_edge(
        L, std::vector<std::optional<anchor_sample_t>>(B));

    unsigned hw = std::thread::hardware_concurrency();
    std::size_t max_workers =
        options_.max_fetch_workers > 0
            ? static_cast<std::size_t>(options_.max_fetch_workers)
            : std::min<std::size_t>({L, hw == 0 ? 1u : hw, 4u});
    max_workers = std::max<std::size_t>(1, std::min(max_workers, L));

    const std::size_t edges_per_worker = (L + max_workers - 1) / max_workers;
    std::vector<std::future<std::vector<std::string>>> futures;
    futures.reserve(max_workers);

    for (std::size_t worker = 0; worker < max_workers; ++worker) {
      const std::size_t edge_begin = worker * edges_per_worker;
      if (edge_begin >= L) {
        break;
      }
      const std::size_t edge_end = std::min(L, edge_begin + edges_per_worker);
      futures.push_back(std::async(
          std::launch::async,
          [&, edge_begin, edge_end]() -> std::vector<std::string> {
            std::vector<std::string> errors;
            for (std::size_t e = edge_begin; e < edge_end; ++e) {
              const auto &edge_id = edge_refs[e].first;
              auto &dataset = *edge_refs[e].second;
              for (std::size_t b = 0; b < B; ++b) {
                const auto anchor_key = anchors[b];
                try {
                  auto sample = dataset.get_by_anchor_key(anchor_key);
                  sample.edge_id = edge_id;
                  by_edge[e][b] = anchor_sample_t{
                      .edge_id = edge_id,
                      .anchor_key = anchor_key,
                      .sample = std::move(sample),
                  };
                } catch (const std::exception &ex) {
                  if (options_.require_all_edges) {
                    std::ostringstream oss;
                    oss << "edge_id=" << edge_id << " anchor_key=" << anchor_key
                        << " error=" << ex.what();
                    errors.push_back(oss.str());
                  }
                }
              }
            }
            return errors;
          }));
    }

    std::vector<std::string> errors;
    for (auto &future : futures) {
      auto worker_errors = future.get();
      errors.insert(errors.end(),
                    std::make_move_iterator(worker_errors.begin()),
                    std::make_move_iterator(worker_errors.end()));
    }
    if (!errors.empty()) {
      TORCH_CHECK(false,
                  "[graph_anchor_edge_dataset_t] parallel edge fetch failed: ",
                  errors.front());
    }

    std::vector<anchor_sample_t> samples;
    samples.reserve(B * L);
    for (std::size_t b = 0; b < B; ++b) {
      for (std::size_t e = 0; e < L; ++e) {
        if (by_edge[e][b].has_value()) {
          samples.push_back(std::move(*by_edge[e][b]));
        }
      }
    }
    return samples;
  }

  void build_anchor_keys_() {
    key_t common_left{};
    key_t common_right{};
    bool first = true;
    for (const auto &edge_id : graph_.edge_ids) {
      const auto found = edge_datasets_.find(edge_id);
      TORCH_CHECK(
          found != edge_datasets_.end(),
          "[graph_anchor_edge_dataset_t] missing edge dataset for edge_id=",
          edge_id);
      const auto &dataset = found->second;
      if (first) {
        common_left = dataset.leftmost_key_value_;
        common_right = dataset.rightmost_key_value_;
        first = false;
      } else {
        common_left = std::max(common_left, dataset.leftmost_key_value_);
        common_right = std::min(common_right, dataset.rightmost_key_value_);
      }
    }
    TORCH_CHECK(!first && common_left <= common_right,
                "[graph_anchor_edge_dataset_t] empty common anchor coverage");

    const auto &reference_dataset = reference_dataset_();
    TORCH_CHECK(reference_dataset.key_value_step_ > 0,
                "[graph_anchor_edge_dataset_t] reference edge key step must be "
                "positive");

    cursor_report_ = graph_anchor_cursor_report_t<key_t>{};
    cursor_report_.graph_order_fingerprint = graph_order_fingerprint_;
    cursor_report_.edge_ids.assign(graph_.edge_ids.begin(),
                                   graph_.edge_ids.end());
    cursor_report_.reference_edge_id =
        options_.reference_edge_id.value_or(graph_.edge_ids.front());
    cursor_report_.require_all_edges = options_.require_all_edges;
    cursor_report_.include_future = options_.include_future;
    cursor_report_.require_future = options_.require_future;
    cursor_report_.validate_anchor_fetch = options_.validate_anchor_fetch;
    cursor_report_.common_left_key = common_left;
    cursor_report_.common_right_key = common_right;
    cursor_report_.reference_left_key = reference_dataset.leftmost_key_value_;
    cursor_report_.reference_right_key = reference_dataset.rightmost_key_value_;
    cursor_report_.reference_key_step = reference_dataset.key_value_step_;

    anchor_keys_.clear();
    anchor_keys_.reserve(reference_dataset.num_records_);
    for (std::size_t i = 0; i < reference_dataset.num_records_; ++i) {
      ++cursor_report_.candidate_anchor_count;
      const key_t anchor_key = static_cast<key_t>(
          reference_dataset.leftmost_key_value_ +
          static_cast<key_t>(i) * reference_dataset.key_value_step_);
      if (anchor_key < common_left || anchor_key > common_right) {
        ++cursor_report_.skipped_outside_common_range;
        continue;
      }
      if (!all_edges_cover_anchor_(anchor_key)) {
        ++cursor_report_.skipped_missing_edge_coverage;
        continue;
      }
      if (!all_edges_fetch_anchor_(anchor_key)) {
        ++cursor_report_.skipped_failed_fetch_probe;
        continue;
      }
      anchor_keys_.push_back(anchor_key);
    }

    if (options_.anchor_order == graph_anchor_order_policy_t::sorted) {
      std::sort(anchor_keys_.begin(), anchor_keys_.end());
    }
    const auto before_unique = anchor_keys_.size();
    anchor_keys_.erase(std::unique(anchor_keys_.begin(), anchor_keys_.end()),
                       anchor_keys_.end());
    cursor_report_.duplicate_anchor_count = before_unique - anchor_keys_.size();
    cursor_report_.accepted_anchor_count = anchor_keys_.size();
    cursor_report_.anchor_keys = anchor_keys_;
    TORCH_CHECK(!anchor_keys_.empty(),
                "[graph_anchor_edge_dataset_t] no valid graph edge anchors");
  }

  cuwacunu::ujcamei::graph::market_graph_t graph_{};
  edge_dataset_map_t edge_datasets_{};
  graph_anchor_edge_dataset_options_t<DatatypeT> options_{};
  std::vector<key_t> anchor_keys_{};
  graph_anchor_cursor_report_t<key_t> cursor_report_{};
  std::string graph_order_fingerprint_{};
  nodelift_compatibility_report_t validation_report_{};
  nodelift_compatibility_identity_t validation_identity_{};
};

} // namespace cuwacunu::ujcamei::source::dataloader
