// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <torch/torch.h>

#include "kikijyeba/composition/config_bundle.h"
#include "ujcamei/source/dataloader/graph_anchor_edge_dataset.h"
#include "ujcamei/source/types/kline_feature_registry.h"
#include "wikimyei/expression/nodelift/srl/stream/node_lifted_stream.h"
#include "wikimyei/inference/expected_value/mdn/mixture_density_network.h"
#include "wikimyei/representation/encoding/vicreg/stream/node_representation_stream.h"
#include "wikimyei/representation/encoding/vicreg/vicreg_rank4_encoder.h"

namespace cuwacunu::kikijyeba::composition {

struct graph_first_pipeline_builder_options_t {
  bool dry_run{false};
  bool force_rebuild_cache{false};
  // 0 derives from config when VICReg and MDN training specs agree.
  std::size_t batch_size{0};
  bool compute_alignment_diagnostics{true};
  bool display_mdn_model{false};
  torch::Dtype dtype{torch::kFloat32};
  torch::Device device{torch::kCPU};
};

struct graph_first_pipeline_dry_run_report_t {
  std::string graph_order_fingerprint{};
  std::string edge_resolution_policy{"explicit_only"};
  std::string edge_source_kind{"real"};
  int64_t node_count{0};
  int64_t edge_count{0};
  int64_t active_channel_count{0};
  std::string fetch_mode{"serial"};
  int64_t max_fetch_workers{0};
  int64_t parallel_min_work_items{16};
  int64_t possible_directed_edge_count{0};
  int64_t available_source_directed_edge_count{0};
  int64_t missing_directed_pair_count{0};
  int64_t available_unselected_edge_count{0};
  int64_t selected_missing_source_edge_count{0};
  int64_t reverse_pair_count{0};
  int64_t selected_missing_reverse_count{0};
  int64_t isolated_node_count{0};
  int64_t connected_component_count{0};
  int64_t selected_cycle_dimension{0};
  int64_t graph_resolution_warning_count{0};
  int64_t input_length{0};
  int64_t future_length{0};
  int64_t feature_width{cuwacunu::ujcamei::source::types::kKlineFeatureWidth};
  std::vector<std::string> node_ids{};
  std::vector<std::string> edge_ids{};
  std::vector<int64_t> target_coords{};
  std::vector<int64_t> required_feature_coords{};
  std::string mask_profile{};
  std::string head_policy{};
  std::string target_domain{};
  std::string activity_target_semantics{};
  int64_t context_dim{0};
  int64_t mixture_count{0};
  int64_t mdn_hidden_width{0};
  std::size_t effective_batch_size{0};
  std::string batch_size_source{};
  std::string dtype{};
  std::string device{};
  int64_t mdn_seed{0};
  int64_t mdn_checkpoint_every{0};
  int64_t mdn_report_every{1};
  int64_t mdn_validation_every{0};
  std::string analytics_status{};

  [[nodiscard]] std::string summary() const {
    std::ostringstream oss;
    oss << "graph_fingerprint=" << graph_order_fingerprint
        << " edge_policy=" << edge_resolution_policy << " nodes=" << node_count
        << " edge_source_kind=" << edge_source_kind << " edges=" << edge_count
        << " active_channels=" << active_channel_count
        << " fetch_mode=" << fetch_mode
        << " max_fetch_workers=" << max_fetch_workers
        << " parallel_min_work_items=" << parallel_min_work_items
        << " available_source_edges=" << available_source_directed_edge_count
        << " missing_pairs=" << missing_directed_pair_count
        << " components=" << connected_component_count
        << " cycle_dimension=" << selected_cycle_dimension
        << " graph_resolution_warnings=" << graph_resolution_warning_count
        << " Hx=" << input_length << " Hf=" << future_length
        << " feature_width=" << feature_width << " context_dim=" << context_dim
        << " head_policy=" << head_policy
        << " batch_size=" << effective_batch_size
        << " batch_size_source=" << batch_size_source << " dtype=" << dtype
        << " device=" << device;
    return oss.str();
  }
};

namespace graph_first_pipeline_builder_detail {

[[nodiscard]] inline std::size_t
checked_batch_size_from_config(int64_t value, const char *field) {
  if (value <= 0 ||
      static_cast<uint64_t>(value) >
          static_cast<uint64_t>(std::numeric_limits<std::size_t>::max())) {
    throw std::runtime_error(std::string("[graph_first_pipeline_builder] ") +
                             field + " must fit in size_t and be positive");
  }
  return static_cast<std::size_t>(value);
}

[[nodiscard]] inline std::size_t
resolve_batch_size(const graph_first_config_bundle_t &bundle,
                   const graph_first_pipeline_builder_options_t &options) {
  if (options.batch_size != 0) {
    return options.batch_size;
  }

  const auto vicreg_batch = checked_batch_size_from_config(
      bundle.vicreg_training.batch_size, "vicreg_training.batch_size");
  const auto mdn_batch = checked_batch_size_from_config(
      bundle.mdn_training.batch_size, "mdn_training.batch_size");
  if (vicreg_batch != mdn_batch) {
    throw std::runtime_error(
        "[graph_first_pipeline_builder] explicit batch_size is required when "
        "VICReg and MDN training specs disagree");
  }
  return mdn_batch;
}

[[nodiscard]] inline torch::Dtype resolve_dtype(const std::string &value) {
  const std::string normalized = cuwacunu::piaabo::parse::simple_kv::lowercase(
      cuwacunu::piaabo::parse::simple_kv::trim(value));
  if (normalized == "float32") {
    return torch::kFloat32;
  }
  if (normalized == "float64") {
    return torch::kFloat64;
  }
  throw std::runtime_error(
      "[graph_first_pipeline_builder] unsupported VICReg DTYPE '" + value +
      "'");
}

[[nodiscard]] inline int64_t
parse_cuda_device_index(const std::string &normalized_device) {
  if (normalized_device.rfind("cuda:", 0) != 0) {
    return -1;
  }
  return cuwacunu::piaabo::parse::simple_kv::parse_i64(
      normalized_device.substr(5));
}

[[nodiscard]] inline torch::Device resolve_device(const std::string &value) {
  const std::string normalized = cuwacunu::piaabo::parse::simple_kv::lowercase(
      cuwacunu::piaabo::parse::simple_kv::trim(value));
  if (normalized == "cpu") {
    return torch::Device(torch::kCPU);
  }
  if (normalized == "cuda" || normalized.rfind("cuda:", 0) == 0) {
    if (!torch::cuda::is_available()) {
      throw std::runtime_error(
          "[graph_first_pipeline_builder] VICReg DEVICE '" + value +
          "' requested CUDA, but CUDA is not available");
    }
    const int64_t cuda_index = parse_cuda_device_index(normalized);
    if (normalized.rfind("cuda:", 0) == 0 && cuda_index < 0) {
      throw std::runtime_error(
          "[graph_first_pipeline_builder] VICReg DEVICE '" + value +
          "' has an invalid CUDA device index");
    }
    const auto cuda_device_count = torch::cuda::device_count();
    if (cuda_index >= 0 &&
        static_cast<std::size_t>(cuda_index) >= cuda_device_count) {
      throw std::runtime_error(
          "[graph_first_pipeline_builder] VICReg DEVICE '" + value +
          "' references a CUDA device index that is not available");
    }
    try {
      return torch::Device(normalized);
    } catch (const std::exception &) {
      throw std::runtime_error(
          "[graph_first_pipeline_builder] unsupported VICReg DEVICE '" + value +
          "'");
    }
  }
  throw std::runtime_error(
      "[graph_first_pipeline_builder] unsupported VICReg DEVICE '" + value +
      "'");
}

[[nodiscard]] inline std::string mask_profile_name(
    cuwacunu::wikimyei::representation::encoding::vicreg::vicreg_mask_profile_t
        profile) {
  namespace vicreg = cuwacunu::wikimyei::representation::encoding::vicreg;
  switch (profile) {
  case vicreg::vicreg_mask_profile_t::all_9:
    return "all_9";
  case vicreg::vicreg_mask_profile_t::price_only:
    return "price_only";
  case vicreg::vicreg_mask_profile_t::close_only:
    return "close_only";
  case vicreg::vicreg_mask_profile_t::activity_only:
    return "activity_only";
  case vicreg::vicreg_mask_profile_t::custom:
    return "custom";
  }
  return "unknown";
}

[[nodiscard]] inline std::string dtype_name(torch::Dtype dtype) {
  if (dtype == torch::kFloat32) {
    return "float32";
  }
  if (dtype == torch::kFloat64) {
    return "float64";
  }
  return "unknown";
}

} // namespace graph_first_pipeline_builder_detail

template <typename DatatypeT> class graph_first_pipeline_builder_t {
public:
  using key_t = typename DatatypeT::key_type_t;
  using graph_source_t =
      cuwacunu::ujcamei::source::dataloader::graph_anchor_edge_dataset_t<
          DatatypeT>;
  using lifted_stream_t = cuwacunu::wikimyei::expression::nodelift::srl::
      stream::node_lifted_stream_t<DatatypeT>;

  graph_first_pipeline_builder_t(
      graph_first_config_bundle_t bundle,
      graph_first_pipeline_builder_options_t options = {})
      : bundle_(std::move(bundle)), options_(std::move(options)) {
    populate_graph_first_source_plan(bundle_);
    validate_graph_first_config_bundle(bundle_);
    batch_size_source_ = options_.batch_size == 0 ? "derived" : "explicit";
    options_.batch_size =
        graph_first_pipeline_builder_detail::resolve_batch_size(bundle_,
                                                                options_);
    options_.dtype = graph_first_pipeline_builder_detail::resolve_dtype(
        bundle_.vicreg.dtype);
    options_.device = graph_first_pipeline_builder_detail::resolve_device(
        bundle_.vicreg.device);
  }

  [[nodiscard]] const graph_first_config_bundle_t &bundle() const {
    return bundle_;
  }

  [[nodiscard]] const graph_first_pipeline_builder_options_t &options() const {
    return options_;
  }

  [[nodiscard]] cuwacunu::ujcamei::graph::market_graph_t market_graph() const {
    return bundle_.source_plan.market_graph;
  }

  [[nodiscard]] cuwacunu::wikimyei::expression::nodelift::srl::graph_t
  srl_graph() const {
    auto market = market_graph();
    cuwacunu::wikimyei::expression::nodelift::srl::graph_t out{};
    out.node_ids = market.node_ids;
    out.edge_ids = market.edge_ids;
    out.base_index = torch::tensor(market.base_index,
                                   torch::TensorOptions().dtype(torch::kInt64));
    out.quote_index = torch::tensor(
        market.quote_index, torch::TensorOptions().dtype(torch::kInt64));
    out.graph_order_fingerprint = market.computed_graph_order_fingerprint();
    return out;
  }

  [[nodiscard]] graph_first_pipeline_dry_run_report_t dry_run_report() const {
    auto market = market_graph();
    graph_first_pipeline_dry_run_report_t out{};
    out.graph_order_fingerprint = market.computed_graph_order_fingerprint();
    out.edge_resolution_policy =
        bundle_.source_resolution_report.edge_resolution_policy;
    out.edge_source_kind = bundle_.source_resolution_report.edge_source_kind;
    out.node_count = static_cast<int64_t>(market.node_ids.size());
    out.edge_count = static_cast<int64_t>(market.edge_ids.size());
    out.active_channel_count = active_channel_count(bundle_.source_dock);
    out.fetch_mode = bundle_.source_resolution_report.fetch_mode;
    out.max_fetch_workers = bundle_.source_resolution_report.max_fetch_workers;
    out.parallel_min_work_items =
        bundle_.source_resolution_report.parallel_min_work_items;
    out.possible_directed_edge_count =
        bundle_.source_resolution_report.possible_directed_edge_count;
    out.available_source_directed_edge_count =
        bundle_.source_resolution_report.available_source_directed_edge_count;
    out.missing_directed_pair_count =
        bundle_.source_resolution_report.missing_directed_pair_count;
    out.available_unselected_edge_count =
        bundle_.source_resolution_report.available_unselected_edge_count;
    out.selected_missing_source_edge_count =
        bundle_.source_resolution_report.selected_missing_source_edge_count;
    out.reverse_pair_count =
        bundle_.source_resolution_report.reverse_pair_count;
    out.selected_missing_reverse_count =
        bundle_.source_resolution_report.selected_missing_reverse_count;
    out.isolated_node_count =
        bundle_.source_resolution_report.isolated_node_count;
    out.connected_component_count =
        bundle_.source_resolution_report.connected_component_count;
    out.selected_cycle_dimension =
        bundle_.source_resolution_report.selected_cycle_dimension;
    out.graph_resolution_warning_count =
        static_cast<int64_t>(bundle_.source_resolution_report.warnings.size());
    out.input_length = max_input_length(bundle_.source_dock);
    out.future_length = max_future_length(bundle_.source_dock);
    out.node_ids = market.node_ids;
    out.edge_ids = market.edge_ids;
    out.target_coords = bundle_.mdn.target_coords;
    out.required_feature_coords = bundle_.vicreg.required_feature_coords;
    out.mask_profile = graph_first_pipeline_builder_detail::mask_profile_name(
        bundle_.vicreg.mask_profile);
    out.head_policy = "per_node";
    out.target_domain = "node_future";
    out.activity_target_semantics = "node_feature_support_mean";
    out.context_dim = context_dim();
    out.mixture_count = bundle_.mdn.mixture_count;
    out.mdn_hidden_width = bundle_.mdn.hidden_width;
    out.effective_batch_size = options_.batch_size;
    out.batch_size_source = batch_size_source_;
    out.dtype = graph_first_pipeline_builder_detail::dtype_name(options_.dtype);
    out.device = options_.device.str();
    out.mdn_seed = bundle_.mdn_training.seed;
    out.mdn_checkpoint_every = bundle_.mdn_training.checkpoint_every;
    out.mdn_report_every = bundle_.mdn_training.report_every;
    out.mdn_validation_every = bundle_.mdn_training.validation_every;
    out.analytics_status =
        bundle_.source_universe.data_analytics_policy.declared
            ? "decoded_validated_not_emitted"
            : "missing";
    return out;
  }

  [[nodiscard]] const std::string &batch_size_source() const {
    return batch_size_source_;
  }

  [[nodiscard]] int64_t context_dim() const {
    return bundle_.vicreg.encoding_dim;
  }

  [[nodiscard]] graph_source_t make_graph_source() const {
    if (options_.dry_run) {
      throw std::runtime_error(
          "[graph_first_pipeline_builder] dry-run mode does not materialize "
          "graph source datasets");
    }
    cuwacunu::ujcamei::source::dataloader::graph_anchor_edge_dataset_options_t<
        DatatypeT>
        source_options{};
    source_options.force_rebuild_cache = options_.force_rebuild_cache;
    source_options.require_normalized = true;
    source_options.include_future = true;
    source_options.require_future = true;
    source_options.fetch_mode =
        bundle_.source_dock.fetch_mode ==
                graph_first_fetch_mode_t::parallel_by_edge
            ? cuwacunu::ujcamei::source::dataloader::fetch_mode_t::
                  parallel_by_edge
            : cuwacunu::ujcamei::source::dataloader::fetch_mode_t::serial;
    source_options.max_fetch_workers = bundle_.source_dock.max_fetch_workers;
    source_options.parallel_min_work_items =
        bundle_.source_dock.parallel_min_work_items;
    typename graph_source_t::source_plan_t source_plan{};
    source_plan.graph = bundle_.source_plan.market_graph;
    source_plan.edge_instruments = bundle_.source_plan.edge_instruments;
    source_plan.materialization_request = cuwacunu::ujcamei::source::storage::
        memory_mapped::make_source_materialization_request(
            bundle_.source_universe, bundle_.source_dock.channel_forms);
    source_plan.validation_source_spec = bundle_.source_plan.compat_source_spec;
    return graph_source_t(std::move(source_plan), std::move(source_options));
  }

  [[nodiscard]] lifted_stream_t
  make_node_lifted_stream(graph_source_t &&source) const {
    if (options_.dry_run) {
      throw std::runtime_error(
          "[graph_first_pipeline_builder] dry-run mode does not materialize "
          "NodeLift streams");
    }
    cuwacunu::wikimyei::expression::nodelift::srl::stream::
        node_lifted_stream_options_t<DatatypeT>
            stream_options{};
    stream_options.batch_size = options_.batch_size;
    stream_options.compute_alignment_diagnostics =
        options_.compute_alignment_diagnostics;
    return lifted_stream_t(std::move(source), srl_graph(),
                           std::move(stream_options));
  }

  [[nodiscard]] cuwacunu::wikimyei::representation::encoding::vicreg::
      VICReg_Rank4_Encoder
      make_vicreg_encoder() const {
    namespace vicreg = cuwacunu::wikimyei::representation::encoding::vicreg;
    return vicreg::VICReg_Rank4_Encoder(
        /*C=*/static_cast<int>(active_channel_count(bundle_.source_dock)),
        /*T=*/
        static_cast<int>(max_input_length(bundle_.source_dock)),
        /*D=*/static_cast<int>(bundle_.vicreg.input_width),
        /*encoding_dim=*/static_cast<int>(bundle_.vicreg.encoding_dim),
        /*channel_expansion_dim=*/
        static_cast<int>(bundle_.vicreg.channel_expansion_dim),
        /*fused_feature_dim=*/
        static_cast<int>(bundle_.vicreg.fused_feature_dim),
        /*hidden_dims=*/static_cast<int>(bundle_.vicreg.encoder_hidden_dim),
        /*depth=*/static_cast<int>(bundle_.vicreg.encoder_depth),
        options_.dtype, options_.device);
  }

  template <typename EncoderT>
  [[nodiscard]] auto
  make_node_representation_stream(lifted_stream_t &&lifted_stream,
                                  EncoderT &encoder) const {
    namespace repstream =
        cuwacunu::wikimyei::representation::encoding::vicreg::stream;
    namespace vicreg = cuwacunu::wikimyei::representation::encoding::vicreg;
    auto adapter_options =
        vicreg::node_adapter_options_from_vicreg_spec(bundle_.vicreg,
                                                      /*training=*/false);
    return repstream::node_representation_stream_t<DatatypeT, EncoderT>(
        std::move(lifted_stream), encoder, adapter_options,
        /*use_swa=*/false, /*detach_to_cpu=*/true);
  }

  [[nodiscard]] cuwacunu::wikimyei::inference::expected_value::mdn::stream::
      mdn_adapter_options_t
      mdn_adapter_options() const {
    return cuwacunu::wikimyei::inference::expected_value::mdn::
        mdn_adapter_options_from_spec(bundle_.mdn);
  }

  [[nodiscard]] std::vector<
      cuwacunu::wikimyei::inference::expected_value::mdn::MdnModel>
  make_mdn_heads(int64_t context_dim, int64_t channel_count,
                 int64_t horizon_count) const {
    if (options_.dry_run) {
      throw std::runtime_error(
          "[graph_first_pipeline_builder] dry-run mode does not materialize "
          "MDN heads");
    }
    const auto market = market_graph();
    const std::size_t head_count = market.node_ids.size();
    std::vector<cuwacunu::wikimyei::inference::expected_value::mdn::MdnModel>
        heads;
    heads.reserve(head_count);
    for (std::size_t i = 0; i < head_count; ++i) {
      heads.emplace_back(
          cuwacunu::wikimyei::inference::expected_value::mdn::MdnModel(
              /*De=*/context_dim,
              /*Df=*/static_cast<int64_t>(bundle_.mdn.target_coords.size()),
              /*C=*/channel_count, /*Hf=*/horizon_count,
              /*K=*/bundle_.mdn.mixture_count,
              /*H=*/bundle_.mdn.hidden_width,
              /*depth=*/bundle_.mdn.residual_depth, options_.dtype,
              options_.device, options_.display_mdn_model));
    }
    return heads;
  }

  [[nodiscard]] static std::vector<torch::Tensor> collect_mdn_head_parameters(
      std::vector<cuwacunu::wikimyei::inference::expected_value::mdn::MdnModel>
          &heads) {
    std::vector<torch::Tensor> params;
    for (auto &head : heads) {
      for (auto &param : head->parameters()) {
        params.push_back(param);
      }
    }
    return params;
  }

private:
  graph_first_config_bundle_t bundle_{};
  graph_first_pipeline_builder_options_t options_{};
  std::string batch_size_source_{"derived"};
};

} // namespace cuwacunu::kikijyeba::composition
