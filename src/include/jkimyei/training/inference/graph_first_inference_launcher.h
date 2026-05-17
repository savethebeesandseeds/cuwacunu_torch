// SPDX-License-Identifier: MIT
#pragma once

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <numeric>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <torch/torch.h>

#include "jkimyei/training/inference/mdn_trainer.h"
#include "kikijyeba/composition/pipeline_builder.h"
#include "wikimyei/inference/expected_value/mdn/stream/mdn_adapter.h"

namespace cuwacunu::jkimyei::training::inference {

struct graph_first_inference_launcher_options_t {
  bool dry_run{false};
  bool write_report{false};
  std::filesystem::path report_path{};
};

struct graph_first_inference_training_report_t {
  std::string training_id{};
  std::string config_path{};
  std::string graph_order_fingerprint{};
  std::vector<std::string> node_ids{};
  std::vector<std::string> edge_ids{};
  std::vector<int64_t> target_coords{};
  int64_t context_dim{0};
  std::string head_policy{};
  std::string target_domain{};
  std::string activity_target_semantics{};
  int64_t mixture_count{0};
  std::size_t effective_batch_size{0};
  std::string batch_size_source{};
  std::string dtype{};
  std::string device{};
  int64_t seed{0};
  std::string seed_scope{};
  int64_t checkpoint_every{0};
  int64_t report_every{1};
  int64_t validation_every{0};
  std::string analytics_status{};
  std::string source_cursor_token{};
  int64_t source_anchor_count{0};
  int64_t source_candidate_anchor_count{0};
  int64_t source_skipped_anchor_count{0};
  std::string source_first_anchor_key{};
  std::string source_last_anchor_key{};

  int64_t steps_attempted{0};
  int64_t steps_completed{0};
  int64_t skipped_batches{0};
  int64_t optimizer_steps{0};
  bool all_target_masks_forced_empty{false};

  double last_loss{std::numeric_limits<double>::quiet_NaN()};
  double mean_loss{std::numeric_limits<double>::quiet_NaN()};
  int64_t last_valid_target_count{0};
  int64_t total_valid_target_count{0};
  int64_t last_valid_row_count{0};
  int64_t total_valid_row_count{0};
  int64_t last_skipped_node_head_count{0};
  int64_t total_skipped_node_head_count{0};
  int64_t last_trained_node_head_count{0};
  int64_t total_trained_node_head_count{0};
  double last_valid_node_target_fraction{
      std::numeric_limits<double>::quiet_NaN()};
  double mean_valid_node_target_fraction{
      std::numeric_limits<double>::quiet_NaN()};
  bool low_target_coverage_warning{false};
  std::vector<int64_t> valid_target_count_by_node{};
  std::vector<double> mean_nll_by_node{};

  double mean_node_mask_coverage{std::numeric_limits<double>::quiet_NaN()};
  double mean_future_mask_coverage{std::numeric_limits<double>::quiet_NaN()};
  double mean_price_residual_energy{std::numeric_limits<double>::quiet_NaN()};
  double mean_node_context_norm_mean{std::numeric_limits<double>::quiet_NaN()};
  double max_node_context_norm_max{std::numeric_limits<double>::quiet_NaN()};
  double mean_mixture_entropy_mean{std::numeric_limits<double>::quiet_NaN()};
  double min_mixture_entropy_min{std::numeric_limits<double>::quiet_NaN()};
  double mean_sigma_mean{std::numeric_limits<double>::quiet_NaN()};
  double min_sigma_min{std::numeric_limits<double>::quiet_NaN()};
  double max_sigma_max{std::numeric_limits<double>::quiet_NaN()};
  double finite_parameter_check{1.0};

  bool checkpoint_written{false};
  int64_t checkpoint_write_count{0};
  int64_t last_checkpoint_optimizer_step{0};
  std::string checkpoint_path{};
  std::string checkpoint_format{};
  bool report_written{false};
  int64_t report_write_count{0};
  int64_t last_report_attempted_step{0};
  std::string report_path{};

  [[nodiscard]] std::string to_text() const {
    std::ostringstream oss;
    oss << "training_id=" << training_id << "\n";
    oss << "config_path=" << config_path << "\n";
    oss << "graph_order_fingerprint=" << graph_order_fingerprint << "\n";
    oss << "context_dim=" << context_dim << "\n";
    oss << "head_policy=" << head_policy << "\n";
    oss << "target_domain=" << target_domain << "\n";
    oss << "activity_target_semantics=" << activity_target_semantics << "\n";
    oss << "mixture_count=" << mixture_count << "\n";
    oss << "effective_batch_size=" << effective_batch_size << "\n";
    oss << "batch_size_source=" << batch_size_source << "\n";
    oss << "dtype=" << dtype << "\n";
    oss << "device=" << device << "\n";
    oss << "seed=" << seed << "\n";
    oss << "seed_scope=" << seed_scope << "\n";
    oss << "checkpoint_every=" << checkpoint_every << "\n";
    oss << "report_every=" << report_every << "\n";
    oss << "validation_every=" << validation_every << "\n";
    oss << "analytics_status=" << analytics_status << "\n";
    oss << "source_cursor_token=" << source_cursor_token << "\n";
    oss << "source_anchor_count=" << source_anchor_count << "\n";
    oss << "source_candidate_anchor_count=" << source_candidate_anchor_count
        << "\n";
    oss << "source_skipped_anchor_count=" << source_skipped_anchor_count
        << "\n";
    oss << "source_first_anchor_key=" << source_first_anchor_key << "\n";
    oss << "source_last_anchor_key=" << source_last_anchor_key << "\n";
    oss << "target_coords=";
    for (std::size_t i = 0; i < target_coords.size(); ++i) {
      if (i != 0) {
        oss << ",";
      }
      oss << target_coords[i];
    }
    oss << "\nedge_ids=";
    for (std::size_t i = 0; i < edge_ids.size(); ++i) {
      if (i != 0) {
        oss << ",";
      }
      oss << edge_ids[i];
    }
    oss << "\nnode_ids=";
    for (std::size_t i = 0; i < node_ids.size(); ++i) {
      if (i != 0) {
        oss << ",";
      }
      oss << node_ids[i];
    }
    oss << "\nsteps_attempted=" << steps_attempted << "\n";
    oss << "steps_completed=" << steps_completed << "\n";
    oss << "skipped_batches=" << skipped_batches << "\n";
    oss << "optimizer_steps=" << optimizer_steps << "\n";
    oss << "all_target_masks_forced_empty="
        << (all_target_masks_forced_empty ? "true" : "false") << "\n";
    oss << "last_loss=" << last_loss << "\n";
    oss << "mean_loss=" << mean_loss << "\n";
    oss << "last_valid_target_count=" << last_valid_target_count << "\n";
    oss << "total_valid_target_count=" << total_valid_target_count << "\n";
    oss << "last_valid_row_count=" << last_valid_row_count << "\n";
    oss << "total_valid_row_count=" << total_valid_row_count << "\n";
    oss << "last_skipped_node_head_count=" << last_skipped_node_head_count
        << "\n";
    oss << "total_skipped_node_head_count=" << total_skipped_node_head_count
        << "\n";
    oss << "last_trained_node_head_count=" << last_trained_node_head_count
        << "\n";
    oss << "total_trained_node_head_count=" << total_trained_node_head_count
        << "\n";
    oss << "last_valid_node_target_fraction=" << last_valid_node_target_fraction
        << "\n";
    oss << "mean_valid_node_target_fraction=" << mean_valid_node_target_fraction
        << "\n";
    oss << "low_target_coverage_warning="
        << (low_target_coverage_warning ? "true" : "false") << "\n";
    oss << "valid_target_count_by_node=";
    for (std::size_t i = 0; i < valid_target_count_by_node.size(); ++i) {
      if (i != 0) {
        oss << ",";
      }
      oss << valid_target_count_by_node[i];
    }
    oss << "\nmean_nll_by_node=";
    for (std::size_t i = 0; i < mean_nll_by_node.size(); ++i) {
      if (i != 0) {
        oss << ",";
      }
      oss << mean_nll_by_node[i];
    }
    oss << "\n";
    oss << "mean_node_mask_coverage=" << mean_node_mask_coverage << "\n";
    oss << "mean_future_mask_coverage=" << mean_future_mask_coverage << "\n";
    oss << "mean_price_residual_energy=" << mean_price_residual_energy << "\n";
    oss << "mean_node_context_norm_mean=" << mean_node_context_norm_mean
        << "\n";
    oss << "max_node_context_norm_max=" << max_node_context_norm_max << "\n";
    oss << "mean_mixture_entropy_mean=" << mean_mixture_entropy_mean << "\n";
    oss << "min_mixture_entropy_min=" << min_mixture_entropy_min << "\n";
    oss << "mean_sigma_mean=" << mean_sigma_mean << "\n";
    oss << "min_sigma_min=" << min_sigma_min << "\n";
    oss << "max_sigma_max=" << max_sigma_max << "\n";
    oss << "finite_parameter_check=" << finite_parameter_check << "\n";
    oss << "checkpoint_written=" << (checkpoint_written ? "true" : "false")
        << "\n";
    oss << "checkpoint_write_count=" << checkpoint_write_count << "\n";
    oss << "last_checkpoint_optimizer_step=" << last_checkpoint_optimizer_step
        << "\n";
    oss << "checkpoint_path=" << checkpoint_path << "\n";
    oss << "checkpoint_format=" << checkpoint_format << "\n";
    oss << "report_written=" << (report_written ? "true" : "false") << "\n";
    oss << "report_write_count=" << report_write_count << "\n";
    oss << "last_report_attempted_step=" << last_report_attempted_step << "\n";
    oss << "report_path=" << report_path << "\n";
    return oss.str();
  }
};

namespace graph_first_inference_launcher_detail {

struct mdn_checkpoint_identity_t {
  std::string graph_order_fingerprint{};
  std::vector<std::string> node_ids{};
  std::vector<int64_t> target_coords{};
  int64_t context_dim{0};
  int64_t mixture_count{0};
};

inline double tensor_mean_or_nan(const torch::Tensor &tensor) {
  if (!tensor.defined() || tensor.numel() == 0) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return tensor.detach()
      .to(torch::kCPU)
      .to(torch::kFloat64)
      .mean()
      .item<double>();
}

inline double bool_fraction_or_nan(const torch::Tensor &mask) {
  if (!mask.defined() || mask.numel() == 0) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return mask.to(torch::kFloat64).mean().to(torch::kCPU).item<double>();
}

inline std::string seed_torch_runtime(uint64_t seed,
                                      const torch::Device &device) {
  torch::manual_seed(seed);
  if (device.is_cuda()) {
    torch::cuda::manual_seed_all(seed);
    return "torch_manual_seed_cuda_manual_seed_all";
  }
  return "torch_manual_seed_cpu";
}

template <typename KeyT>
inline std::string optional_key_to_string(const std::optional<KeyT> &value) {
  if (!value.has_value()) {
    return {};
  }
  std::ostringstream oss;
  oss << *value;
  return oss.str();
}

template <typename KeyT>
inline void
move_mdn_input_batch_to_device(cuwacunu::wikimyei::inference::expected_value::
                                   mdn::stream::mdn_input_batch_t<KeyT> &batch,
                               torch::Dtype dtype,
                               const torch::Device &device) {
  const auto value_options = torch::TensorOptions().dtype(dtype).device(device);
  const auto bool_options =
      torch::TensorOptions().dtype(torch::kBool).device(device);
  const auto index_options =
      torch::TensorOptions().dtype(torch::kInt64).device(device);

  if (batch.context.defined()) {
    batch.context = batch.context.to(value_options);
  }
  if (batch.context_mask.defined()) {
    batch.context_mask = batch.context_mask.to(bool_options);
  }
  if (batch.future.defined()) {
    batch.future = batch.future.to(value_options);
  }
  if (batch.future_mask.defined()) {
    batch.future_mask = batch.future_mask.to(bool_options);
  }
  if (batch.node_index.defined()) {
    batch.node_index = batch.node_index.to(index_options);
  }
  if (batch.anchor_index.defined()) {
    batch.anchor_index = batch.anchor_index.to(index_options);
  }
  if (batch.anchor_keys.defined()) {
    batch.anchor_keys = batch.anchor_keys.to(index_options);
  }
}

inline void ensure_parent_directory(const std::filesystem::path &path,
                                    const char *path_kind) {
  const auto parent = path.parent_path();
  if (parent.empty()) {
    return;
  }
  std::error_code ec;
  std::filesystem::create_directories(parent, ec);
  if (ec) {
    throw std::runtime_error(
        std::string("[graph_first_inference_launcher] failed to create ") +
        path_kind + " parent directory '" + parent.string() +
        "': " + ec.message());
  }
}

inline void
write_report_file(const std::filesystem::path &path,
                  const graph_first_inference_training_report_t &report) {
  if (path.empty()) {
    throw std::runtime_error(
        "[graph_first_inference_launcher] report path is empty");
  }
  ensure_parent_directory(path, "report");
  std::ofstream out(path, std::ios::trunc);
  if (!out) {
    throw std::runtime_error(
        "[graph_first_inference_launcher] failed to open report path");
  }
  out << report.to_text();
}

inline torch::Tensor
int64_tensor_from_vector(const std::vector<int64_t> &values) {
  const auto opts = torch::TensorOptions().dtype(torch::kInt64);
  if (values.empty()) {
    return torch::empty({0}, opts);
  }
  return torch::tensor(values, opts);
}

inline std::pair<std::vector<int64_t>, std::vector<int64_t>>
string_list_to_lengths_and_bytes(const std::vector<std::string> &values) {
  std::vector<int64_t> lengths;
  std::vector<int64_t> bytes;
  lengths.reserve(values.size());
  for (const auto &value : values) {
    lengths.push_back(static_cast<int64_t>(value.size()));
    for (const unsigned char ch : value) {
      bytes.push_back(static_cast<int64_t>(ch));
    }
  }
  return {std::move(lengths), std::move(bytes)};
}

inline std::vector<int64_t> string_to_bytes(const std::string &value) {
  std::vector<int64_t> bytes;
  bytes.reserve(value.size());
  for (const unsigned char ch : value) {
    bytes.push_back(static_cast<int64_t>(ch));
  }
  return bytes;
}

inline std::vector<int64_t>
tensor_to_int64_vector(const torch::Tensor &tensor) {
  TORCH_CHECK(tensor.defined() && tensor.dim() == 1,
              "[graph_first_inference_launcher] expected rank-1 metadata "
              "tensor");
  auto cpu = tensor.to(torch::kCPU).to(torch::kInt64).contiguous();
  std::vector<int64_t> out;
  out.reserve(cpu.numel());
  auto accessor = cpu.accessor<int64_t, 1>();
  for (int64_t i = 0; i < cpu.numel(); ++i) {
    out.push_back(accessor[i]);
  }
  return out;
}

inline mdn_checkpoint_identity_t checkpoint_identity_from_report(
    const graph_first_inference_training_report_t &report) {
  mdn_checkpoint_identity_t out{};
  out.graph_order_fingerprint = report.graph_order_fingerprint;
  out.node_ids = report.node_ids;
  out.target_coords = report.target_coords;
  out.context_dim = report.context_dim;
  out.mixture_count = report.mixture_count;
  return out;
}

inline void save_mdn_checkpoint_file(
    const std::filesystem::path &path,
    const graph_first_inference_training_report_t &report,
    const std::vector<
        cuwacunu::wikimyei::inference::expected_value::mdn::MdnModel> &heads,
    torch::optim::Optimizer &optimizer) {
  if (path.empty()) {
    return;
  }
  ensure_parent_directory(path, "checkpoint");
  TORCH_CHECK(!heads.empty(),
              "[graph_first_inference_launcher] cannot checkpoint empty MDN "
              "head set");

  torch::serialize::OutputArchive root;
  for (std::size_t i = 0; i < heads.size(); ++i) {
    torch::serialize::OutputArchive head_archive;
    heads[i]->save(head_archive);
    root.write("node_head_" + std::to_string(i), head_archive);
  }

  torch::serialize::OutputArchive optimizer_archive;
  optimizer.save(optimizer_archive);
  root.write("optimizer", optimizer_archive);

  const auto i64 = torch::TensorOptions().dtype(torch::kInt64);
  root.write("meta/node_count",
             torch::tensor({static_cast<int64_t>(heads.size())}, i64));
  root.write("meta/context_dim", torch::tensor({report.context_dim}, i64));
  root.write("meta/mixture_count", torch::tensor({report.mixture_count}, i64));
  root.write("meta/optimizer_steps",
             torch::tensor({report.optimizer_steps}, i64));
  root.write("meta/target_coords",
             int64_tensor_from_vector(report.target_coords));
  root.write("meta/graph_order_fingerprint_bytes",
             int64_tensor_from_vector(
                 string_to_bytes(report.graph_order_fingerprint)));
  auto [node_id_lengths, node_id_bytes] =
      string_list_to_lengths_and_bytes(report.node_ids);
  root.write("meta/node_id_lengths", int64_tensor_from_vector(node_id_lengths));
  root.write("meta/node_id_bytes", int64_tensor_from_vector(node_id_bytes));
  root.save_to(path.string());
}

inline void load_mdn_checkpoint_file(
    const std::filesystem::path &path,
    std::vector<cuwacunu::wikimyei::inference::expected_value::mdn::MdnModel>
        &heads,
    torch::optim::Optimizer *optimizer = nullptr,
    const mdn_checkpoint_identity_t *expected_identity = nullptr) {
  TORCH_CHECK(!path.empty(),
              "[graph_first_inference_launcher] checkpoint path is empty");
  TORCH_CHECK(!heads.empty(),
              "[graph_first_inference_launcher] cannot load into empty MDN "
              "head set");

  torch::serialize::InputArchive root;
  root.load_from(path.string(), torch::Device(torch::kCPU));

  torch::Tensor saved_node_count{};
  root.read("meta/node_count", saved_node_count);
  TORCH_CHECK(saved_node_count.defined() && saved_node_count.numel() == 1,
              "[graph_first_inference_launcher] checkpoint missing node_count "
              "metadata");
  TORCH_CHECK(saved_node_count.item<int64_t>() ==
                  static_cast<int64_t>(heads.size()),
              "[graph_first_inference_launcher] checkpoint node_count does "
              "not match current node head count");

  torch::Tensor saved_context_dim{};
  torch::Tensor saved_mixture_count{};
  torch::Tensor saved_target_coords{};
  torch::Tensor saved_graph_fingerprint{};
  torch::Tensor saved_node_id_lengths{};
  torch::Tensor saved_node_id_bytes{};
  root.read("meta/context_dim", saved_context_dim);
  root.read("meta/mixture_count", saved_mixture_count);
  root.read("meta/target_coords", saved_target_coords);
  root.read("meta/graph_order_fingerprint_bytes", saved_graph_fingerprint);
  root.read("meta/node_id_lengths", saved_node_id_lengths);
  root.read("meta/node_id_bytes", saved_node_id_bytes);

  if (expected_identity != nullptr) {
    TORCH_CHECK(saved_context_dim.defined() && saved_context_dim.numel() == 1 &&
                    saved_context_dim.item<int64_t>() ==
                        expected_identity->context_dim,
                "[graph_first_inference_launcher] checkpoint context_dim does "
                "not match current MDN context");
    TORCH_CHECK(saved_mixture_count.defined() &&
                    saved_mixture_count.numel() == 1 &&
                    saved_mixture_count.item<int64_t>() ==
                        expected_identity->mixture_count,
                "[graph_first_inference_launcher] checkpoint mixture_count "
                "does not match current MDN config");

    const auto expected_target_coords = expected_identity->target_coords;
    TORCH_CHECK(tensor_to_int64_vector(saved_target_coords) ==
                    expected_target_coords,
                "[graph_first_inference_launcher] checkpoint target_coords do "
                "not match current MDN target policy");

    TORCH_CHECK(tensor_to_int64_vector(saved_graph_fingerprint) ==
                    string_to_bytes(expected_identity->graph_order_fingerprint),
                "[graph_first_inference_launcher] checkpoint graph fingerprint "
                "does not match current graph");

    auto [expected_node_lengths, expected_node_bytes] =
        string_list_to_lengths_and_bytes(expected_identity->node_ids);
    TORCH_CHECK(tensor_to_int64_vector(saved_node_id_lengths) ==
                    expected_node_lengths,
                "[graph_first_inference_launcher] checkpoint node_id lengths "
                "do not match current node order");
    TORCH_CHECK(tensor_to_int64_vector(saved_node_id_bytes) ==
                    expected_node_bytes,
                "[graph_first_inference_launcher] checkpoint node_ids do not "
                "match current node order");
  }

  for (std::size_t i = 0; i < heads.size(); ++i) {
    torch::serialize::InputArchive head_archive;
    root.read("node_head_" + std::to_string(i), head_archive);
    heads[i]->load(head_archive);
  }

  if (optimizer != nullptr) {
    torch::serialize::InputArchive optimizer_archive;
    root.read("optimizer", optimizer_archive);
    optimizer->load(optimizer_archive);
  }
}

} // namespace graph_first_inference_launcher_detail

template <typename DatatypeT> class graph_first_inference_launcher_t {
public:
  using builder_t =
      cuwacunu::kikijyeba::composition::graph_first_pipeline_builder_t<
          DatatypeT>;

  graph_first_inference_launcher_t(
      builder_t builder, graph_first_inference_launcher_options_t options = {})
      : builder_(std::move(builder)), options_(std::move(options)) {
    validate_batch_size_contract();
  }

  [[nodiscard]] graph_first_inference_training_report_t dry_run_report() const {
    const auto plan = builder_.dry_run_report();
    graph_first_inference_training_report_t out{};
    out.training_id = builder_.bundle().mdn_training.training_id;
    out.config_path = builder_.bundle().config_path;
    out.graph_order_fingerprint = plan.graph_order_fingerprint;
    out.node_ids = plan.node_ids;
    out.edge_ids = plan.edge_ids;
    out.target_coords = plan.target_coords;
    out.context_dim = plan.context_dim;
    out.head_policy = plan.head_policy;
    out.target_domain = plan.target_domain;
    out.activity_target_semantics = plan.activity_target_semantics;
    out.mixture_count = plan.mixture_count;
    out.effective_batch_size = plan.effective_batch_size;
    out.batch_size_source = plan.batch_size_source;
    out.dtype = plan.dtype;
    out.device = plan.device;
    out.seed = plan.mdn_seed;
    out.seed_scope = builder_.options().device.is_cuda()
                         ? "torch_manual_seed_cuda_manual_seed_all"
                         : "torch_manual_seed_cpu";
    out.checkpoint_every = plan.mdn_checkpoint_every;
    out.report_every = plan.mdn_report_every;
    out.validation_every = plan.mdn_validation_every;
    out.analytics_status = plan.analytics_status;
    return out;
  }

  [[nodiscard]] graph_first_inference_training_report_t run() const {
    if (options_.dry_run) {
      return dry_run_report();
    }

    const auto checkpoint_every =
        builder_.bundle().mdn_training.checkpoint_every;
    const auto report_every = builder_.bundle().mdn_training.report_every;
    if ((options_.write_report || checkpoint_every > 0) &&
        options_.report_path.empty()) {
      throw std::runtime_error(
          "[graph_first_inference_launcher] report_path is required for "
          "configured report or checkpoint output");
    }

    const auto seed_scope =
        graph_first_inference_launcher_detail::seed_torch_runtime(
            static_cast<uint64_t>(builder_.bundle().mdn_training.seed),
            builder_.options().device);

    auto source = builder_.make_graph_source();
    const auto source_cursor_report = source.cursor_report();
    auto srl_graph = builder_.srl_graph();
    auto lifted_stream = builder_.make_node_lifted_stream(std::move(source));
    auto encoder = builder_.make_vicreg_encoder();
    auto representation_stream = builder_.make_node_representation_stream(
        std::move(lifted_stream), encoder);

    graph_first_inference_training_report_t report = dry_run_report();
    report.seed_scope = seed_scope;
    report.all_target_masks_forced_empty = force_empty_targets_for_test_;
    report.source_cursor_token = source_cursor_report.cursor_token();
    report.source_anchor_count =
        static_cast<int64_t>(source_cursor_report.accepted_anchor_count);
    report.source_candidate_anchor_count =
        static_cast<int64_t>(source_cursor_report.candidate_anchor_count);
    report.source_skipped_anchor_count =
        static_cast<int64_t>(source_cursor_report.skipped_anchor_count());
    report.source_first_anchor_key =
        graph_first_inference_launcher_detail::optional_key_to_string(
            source_cursor_report.first_anchor_key());
    report.source_last_anchor_key =
        graph_first_inference_launcher_detail::optional_key_to_string(
            source_cursor_report.last_anchor_key());
    const auto training_options =
        cuwacunu::jkimyei::training::mdn_training_options_from_specs(
            builder_.bundle().mdn_training, builder_.bundle().mdn);

    std::vector<cuwacunu::wikimyei::inference::expected_value::mdn::MdnModel>
        heads;
    std::unique_ptr<torch::optim::Adam> optimizer;

    double loss_sum = 0.0;
    int64_t loss_count = 0;
    double node_mask_sum = 0.0;
    double future_mask_sum = 0.0;
    double residual_energy_sum = 0.0;
    int64_t diagnostics_count = 0;
    double node_context_norm_mean_sum = 0.0;
    int64_t node_context_norm_mean_count = 0;
    double node_context_norm_max = -std::numeric_limits<double>::infinity();
    double mixture_entropy_mean_sum = 0.0;
    int64_t mixture_entropy_mean_count = 0;
    double mixture_entropy_min = std::numeric_limits<double>::infinity();
    double sigma_mean_sum = 0.0;
    int64_t sigma_mean_count = 0;
    double sigma_min = std::numeric_limits<double>::infinity();
    double sigma_max = -std::numeric_limits<double>::infinity();
    double valid_node_target_fraction_sum = 0.0;
    int64_t valid_node_target_fraction_count = 0;
    std::vector<double> nll_by_node_sum;
    std::vector<int64_t> nll_by_node_count;

    auto refresh_running_report = [&]() {
      if (loss_count > 0) {
        report.mean_loss = loss_sum / static_cast<double>(loss_count);
      }
      if (diagnostics_count > 0) {
        report.mean_node_mask_coverage =
            node_mask_sum / static_cast<double>(diagnostics_count);
        report.mean_future_mask_coverage =
            future_mask_sum / static_cast<double>(diagnostics_count);
        report.mean_price_residual_energy =
            residual_energy_sum / static_cast<double>(diagnostics_count);
      }
      if (valid_node_target_fraction_count > 0) {
        report.mean_valid_node_target_fraction =
            valid_node_target_fraction_sum /
            static_cast<double>(valid_node_target_fraction_count);
      }
      if (node_context_norm_mean_count > 0) {
        report.mean_node_context_norm_mean =
            node_context_norm_mean_sum /
            static_cast<double>(node_context_norm_mean_count);
      }
      if (std::isfinite(node_context_norm_max)) {
        report.max_node_context_norm_max = node_context_norm_max;
      }
      if (mixture_entropy_mean_count > 0) {
        report.mean_mixture_entropy_mean =
            mixture_entropy_mean_sum /
            static_cast<double>(mixture_entropy_mean_count);
      }
      if (std::isfinite(mixture_entropy_min)) {
        report.min_mixture_entropy_min = mixture_entropy_min;
      }
      if (sigma_mean_count > 0) {
        report.mean_sigma_mean =
            sigma_mean_sum / static_cast<double>(sigma_mean_count);
      }
      if (std::isfinite(sigma_min)) {
        report.min_sigma_min = sigma_min;
      }
      if (std::isfinite(sigma_max)) {
        report.max_sigma_max = sigma_max;
      }
      if (!nll_by_node_sum.empty()) {
        report.mean_nll_by_node.assign(
            nll_by_node_sum.size(), std::numeric_limits<double>::quiet_NaN());
        for (std::size_t i = 0; i < nll_by_node_sum.size(); ++i) {
          if (nll_by_node_count[i] > 0) {
            report.mean_nll_by_node[i] =
                nll_by_node_sum[i] / static_cast<double>(nll_by_node_count[i]);
          }
        }
      }
    };

    auto write_checkpoint = [&]() {
      if (heads.empty() || !optimizer) {
        return;
      }
      refresh_running_report();
      auto checkpoint_path = options_.report_path;
      checkpoint_path += ".mdn.pt";
      report.checkpoint_written = true;
      ++report.checkpoint_write_count;
      report.last_checkpoint_optimizer_step = report.optimizer_steps;
      report.checkpoint_path = checkpoint_path.string();
      report.checkpoint_format = "torch_archive_mdn_v1";
      graph_first_inference_launcher_detail::save_mdn_checkpoint_file(
          checkpoint_path, report, heads, *optimizer);
    };

    auto write_report = [&]() {
      if (!options_.write_report) {
        return;
      }
      refresh_running_report();
      report.report_written = true;
      ++report.report_write_count;
      report.last_report_attempted_step = report.steps_attempted;
      report.report_path = options_.report_path.string();
      graph_first_inference_launcher_detail::write_report_file(
          options_.report_path, report);
    };

    const int64_t max_steps = builder_.bundle().mdn_training.max_steps;
    while (representation_stream.has_next() &&
           (max_steps == 0 || report.steps_attempted < max_steps)) {
      ++report.steps_attempted;
      auto node_batch = representation_stream.next();
      auto mdn_batch =
          cuwacunu::wikimyei::inference::expected_value::mdn::stream::
              make_mdn_input_batch(node_batch, builder_.mdn_adapter_options());
      graph_first_inference_launcher_detail::move_mdn_input_batch_to_device(
          mdn_batch, builder_.options().dtype, builder_.options().device);
      if (force_empty_targets_for_test_) {
        mdn_batch.future_mask.fill_(false);
      }

      if (heads.empty()) {
        heads = builder_.make_mdn_heads(
            /*context_dim=*/mdn_batch.context.size(1),
            /*channel_count=*/mdn_batch.future.size(1),
            /*horizon_count=*/mdn_batch.future.size(2));
        auto params = builder_t::collect_mdn_head_parameters(heads);
        optimizer = std::make_unique<torch::optim::Adam>(
            params, torch::optim::AdamOptions(
                        builder_.bundle().mdn_training.learning_rate));
      }

      auto step_report =
          train_mdn_batch(mdn_batch, heads, *optimizer, training_options);
      if (step_report.trained) {
        ++report.steps_completed;
        report.optimizer_steps += step_report.optimizer_step_applied ? 1 : 0;
        report.last_loss = step_report.loss;
        loss_sum += step_report.loss;
        ++loss_count;
      } else if (step_report.skipped_empty) {
        ++report.skipped_batches;
      }
      report.last_valid_target_count = step_report.valid_target_count;
      report.total_valid_target_count += step_report.valid_target_count;
      report.last_valid_row_count = step_report.valid_row_count;
      report.total_valid_row_count += step_report.valid_row_count;
      report.last_skipped_node_head_count = step_report.skipped_node_head_count;
      report.total_skipped_node_head_count +=
          step_report.skipped_node_head_count;
      report.last_trained_node_head_count = step_report.trained_node_head_count;
      report.total_trained_node_head_count +=
          step_report.trained_node_head_count;
      report.last_valid_node_target_fraction =
          step_report.valid_node_target_fraction;
      if (std::isfinite(step_report.valid_node_target_fraction)) {
        valid_node_target_fraction_sum +=
            step_report.valid_node_target_fraction;
        ++valid_node_target_fraction_count;
      }
      report.low_target_coverage_warning =
          report.low_target_coverage_warning ||
          step_report.low_target_coverage_warning;
      if (report.valid_target_count_by_node.empty()) {
        report.valid_target_count_by_node.assign(report.node_ids.size(), 0);
      }
      for (std::size_t i = 0; i < step_report.valid_target_count_by_node.size();
           ++i) {
        if (i < report.valid_target_count_by_node.size()) {
          report.valid_target_count_by_node[i] +=
              step_report.valid_target_count_by_node[i];
        }
      }
      if (nll_by_node_sum.empty()) {
        nll_by_node_sum.assign(report.node_ids.size(), 0.0);
        nll_by_node_count.assign(report.node_ids.size(), 0);
      }
      for (std::size_t i = 0; i < step_report.nll_by_node.size(); ++i) {
        if (i < nll_by_node_sum.size() &&
            std::isfinite(step_report.nll_by_node[i])) {
          nll_by_node_sum[i] += step_report.nll_by_node[i];
          ++nll_by_node_count[i];
        }
      }
      if (std::isfinite(step_report.node_context_norm_mean)) {
        node_context_norm_mean_sum += step_report.node_context_norm_mean;
        ++node_context_norm_mean_count;
      }
      if (std::isfinite(step_report.node_context_norm_max)) {
        node_context_norm_max =
            std::max(node_context_norm_max, step_report.node_context_norm_max);
      }
      if (std::isfinite(step_report.mixture_entropy_mean)) {
        mixture_entropy_mean_sum += step_report.mixture_entropy_mean;
        ++mixture_entropy_mean_count;
      }
      if (std::isfinite(step_report.mixture_entropy_min)) {
        mixture_entropy_min =
            std::min(mixture_entropy_min, step_report.mixture_entropy_min);
      }
      if (std::isfinite(step_report.sigma_mean)) {
        sigma_mean_sum += step_report.sigma_mean;
        ++sigma_mean_count;
      }
      if (std::isfinite(step_report.sigma_min)) {
        sigma_min = std::min(sigma_min, step_report.sigma_min);
      }
      if (std::isfinite(step_report.sigma_max)) {
        sigma_max = std::max(sigma_max, step_report.sigma_max);
      }
      report.finite_parameter_check =
          (report.finite_parameter_check != 0.0 &&
           step_report.finite_parameter_check != 0.0)
              ? 1.0
              : 0.0;

      const double node_mask_fraction =
          graph_first_inference_launcher_detail::bool_fraction_or_nan(
              node_batch.node_encoding_mask);
      const double future_mask_fraction =
          graph_first_inference_launcher_detail::bool_fraction_or_nan(
              mdn_batch.future_mask);
      const double residual_energy =
          graph_first_inference_launcher_detail::tensor_mean_or_nan(
              node_batch.price_residual.square().masked_fill(
                  node_batch.price_residual_mask.to(torch::kBool).logical_not(),
                  0.0));
      if (std::isfinite(node_mask_fraction)) {
        node_mask_sum += node_mask_fraction;
      }
      if (std::isfinite(future_mask_fraction)) {
        future_mask_sum += future_mask_fraction;
      }
      if (std::isfinite(residual_energy)) {
        residual_energy_sum += residual_energy;
      }
      ++diagnostics_count;

      if (step_report.optimizer_step_applied && checkpoint_every > 0 &&
          report.optimizer_steps % checkpoint_every == 0) {
        write_checkpoint();
      }
      if (report_every > 0 && report.steps_attempted % report_every == 0) {
        write_report();
      }
    }

    refresh_running_report();
    if (!options_.write_report ||
        report.last_report_attempted_step != report.steps_attempted) {
      write_report();
    }

    return report;
  }

  void set_force_empty_targets_for_test(bool value) {
    force_empty_targets_for_test_ = value;
  }

private:
  void validate_batch_size_contract() const {
    const auto expected =
        static_cast<std::size_t>(builder_.bundle().mdn_training.batch_size);
    if (builder_.options().batch_size != expected) {
      throw std::runtime_error(
          "[graph_first_inference_launcher] builder batch_size must match "
          "MDN training BATCH_SIZE");
    }
  }

  builder_t builder_;
  graph_first_inference_launcher_options_t options_{};
  bool force_empty_targets_for_test_{false};
};

} // namespace cuwacunu::jkimyei::training::inference
