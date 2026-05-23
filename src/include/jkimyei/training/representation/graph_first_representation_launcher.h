// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <torch/torch.h>

#include "kikijyeba/lattice/runtime_report/component_runtime_lls.h"
#include "kikijyeba/protocol/component_stream.h"
#include "kikijyeba/protocol/pipeline_builder.h"
#include "kikijyeba/topology/dock_binding.h"
#include "wikimyei/assembly.h"
#include "wikimyei/representation/encoding/vicreg/node_stream_adapter.h"
#include "wikimyei/representation/encoding/vicreg/vicreg_node_train_model.h"

namespace cuwacunu::jkimyei::training::representation {

struct graph_first_representation_launcher_options_t {
  bool dry_run{false};
  bool train_target_from_wave{true};
  bool train_target{true};
  bool write_report{false};
  std::filesystem::path report_path{};
  cuwacunu::kikijyeba::lattice::runtime_report::runtime_report_mode_t
      runtime_report_mode{cuwacunu::kikijyeba::lattice::runtime_report::
                              runtime_report_mode_t::normal};
};

struct graph_first_representation_training_report_t {
  std::string training_id{};
  std::string config_path{};
  std::string component_id{};
  std::string representation_architecture{
      "global_fused_temporal_node_encoder.v1"};
  std::string representation_contract{"graph_order.node_representation.v1"};
  std::string primary_value_shape{"[B,N,D_e] or [B,N,Hx,D_e]"};
  std::string channel_axis_policy{"fused_before_primary_output"};
  std::string graph_order_fingerprint{};
  std::vector<std::string> node_ids{};
  std::vector<std::string> edge_ids{};
  std::vector<int64_t> required_feature_coords{};
  std::string mask_profile{};
  int64_t encoding_dim{0};
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
  std::string wave_id{};
  std::string wave_mode{};
  std::string target_action{};
  std::string wave_source_cursor_kind{};
  std::string wave_source_cursor_scope{};
  std::string wave_source_range_policy{};
  int64_t requested_anchor_index_begin{-1};
  int64_t requested_anchor_index_end{-1};
  std::string runtime_report_mode{};
  std::string stream_plan{};
  std::string source_cursor_token{};
  int64_t source_anchor_count{0};
  int64_t source_candidate_anchor_count{0};
  int64_t source_skipped_anchor_count{0};
  int64_t source_skipped_outside_common_range{0};
  int64_t source_skipped_missing_edge_coverage{0};
  int64_t source_skipped_failed_fetch_probe{0};
  int64_t source_duplicate_anchor_count{0};
  double source_accepted_anchor_fraction{
      std::numeric_limits<double>::quiet_NaN()};
  std::string source_anchor_domain_warning_level{"none"};
  std::string source_anchor_domain_warnings{};
  std::string source_common_left_key{};
  std::string source_common_right_key{};
  std::string source_reference_left_key{};
  std::string source_reference_right_key{};
  std::string source_first_anchor_key{};
  std::string source_last_anchor_key{};
  int64_t wave_streamed_anchor_count{0};
  std::string wave_first_anchor_key{};
  std::string wave_last_anchor_key{};
  int64_t wave_pulses_attempted{0};
  int64_t wave_pulses_completed{0};
  int64_t wave_pulses_skipped{0};

  int64_t steps_attempted{0};
  int64_t steps_completed{0};
  int64_t skipped_batches{0};
  int64_t optimizer_steps{0};

  double last_loss{std::numeric_limits<double>::quiet_NaN()};
  double mean_loss{std::numeric_limits<double>::quiet_NaN()};
  double last_invariance_loss{std::numeric_limits<double>::quiet_NaN()};
  double mean_invariance_loss{std::numeric_limits<double>::quiet_NaN()};
  double last_variance_loss{std::numeric_limits<double>::quiet_NaN()};
  double mean_variance_loss{std::numeric_limits<double>::quiet_NaN()};
  double last_covariance_loss{std::numeric_limits<double>::quiet_NaN()};
  double mean_covariance_loss{std::numeric_limits<double>::quiet_NaN()};
  double last_grad_norm{std::numeric_limits<double>::quiet_NaN()};
  double max_grad_norm{std::numeric_limits<double>::quiet_NaN()};
  int64_t last_valid_projection_rows{0};
  int64_t total_valid_projection_rows{0};

  int64_t adapter_original_rows{0};
  int64_t adapter_kept_rows{0};
  int64_t adapter_dropped_rows{0};
  int64_t adapter_valid_channel_time_count{0};
  int64_t adapter_kept_valid_channel_time_count{0};
  double mean_adapter_valid_channel_time_fraction{
      std::numeric_limits<double>::quiet_NaN()};
  double mean_adapter_kept_valid_channel_time_fraction{
      std::numeric_limits<double>::quiet_NaN()};
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
  bool runtime_lls_emitted{false};
  std::string nodelift_runtime_lls{};
  std::string representation_training_runtime_lls{};

  [[nodiscard]] std::string to_text() const {
    std::ostringstream oss;
    oss << "training_id=" << training_id << "\n";
    oss << "config_path=" << config_path << "\n";
    oss << "component_id=" << component_id << "\n";
    oss << "representation_architecture=" << representation_architecture
        << "\n";
    oss << "representation_contract=" << representation_contract << "\n";
    oss << "primary_value_shape=" << primary_value_shape << "\n";
    oss << "channel_axis_policy=" << channel_axis_policy << "\n";
    oss << "graph_order_fingerprint=" << graph_order_fingerprint << "\n";
    oss << "encoding_dim=" << encoding_dim << "\n";
    oss << "mask_profile=" << mask_profile << "\n";
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
    oss << "wave_id=" << wave_id << "\n";
    oss << "wave_mode=" << wave_mode << "\n";
    oss << "target_action=" << target_action << "\n";
    oss << "wave_source_cursor_kind=" << wave_source_cursor_kind << "\n";
    oss << "wave_source_cursor_scope=" << wave_source_cursor_scope << "\n";
    oss << "wave_source_range_policy=" << wave_source_range_policy << "\n";
    oss << "requested_anchor_index_begin=" << requested_anchor_index_begin
        << "\n";
    oss << "requested_anchor_index_end=" << requested_anchor_index_end << "\n";
    oss << "runtime_report_mode=" << runtime_report_mode << "\n";
    oss << "stream_plan=" << stream_plan << "\n";
    oss << "source_cursor_token=" << source_cursor_token << "\n";
    oss << "source_anchor_count=" << source_anchor_count << "\n";
    oss << "source_candidate_anchor_count=" << source_candidate_anchor_count
        << "\n";
    oss << "source_skipped_anchor_count=" << source_skipped_anchor_count
        << "\n";
    oss << "source_skipped_outside_common_range="
        << source_skipped_outside_common_range << "\n";
    oss << "source_skipped_missing_edge_coverage="
        << source_skipped_missing_edge_coverage << "\n";
    oss << "source_skipped_failed_fetch_probe="
        << source_skipped_failed_fetch_probe << "\n";
    oss << "source_duplicate_anchor_count=" << source_duplicate_anchor_count
        << "\n";
    oss << "source_accepted_anchor_fraction=" << source_accepted_anchor_fraction
        << "\n";
    oss << "source_anchor_domain_warning_level="
        << source_anchor_domain_warning_level << "\n";
    oss << "source_anchor_domain_warnings=" << source_anchor_domain_warnings
        << "\n";
    oss << "source_common_left_key=" << source_common_left_key << "\n";
    oss << "source_common_right_key=" << source_common_right_key << "\n";
    oss << "source_reference_left_key=" << source_reference_left_key << "\n";
    oss << "source_reference_right_key=" << source_reference_right_key << "\n";
    oss << "source_first_anchor_key=" << source_first_anchor_key << "\n";
    oss << "source_last_anchor_key=" << source_last_anchor_key << "\n";
    oss << "wave_streamed_anchor_count=" << wave_streamed_anchor_count << "\n";
    oss << "wave_first_anchor_key=" << wave_first_anchor_key << "\n";
    oss << "wave_last_anchor_key=" << wave_last_anchor_key << "\n";
    oss << "wave_pulses_attempted=" << wave_pulses_attempted << "\n";
    oss << "wave_pulses_completed=" << wave_pulses_completed << "\n";
    oss << "wave_pulses_skipped=" << wave_pulses_skipped << "\n";
    oss << "required_feature_coords=";
    for (std::size_t i = 0; i < required_feature_coords.size(); ++i) {
      if (i != 0) {
        oss << ",";
      }
      oss << required_feature_coords[i];
    }
    oss << "\nnode_ids=";
    for (std::size_t i = 0; i < node_ids.size(); ++i) {
      if (i != 0) {
        oss << ",";
      }
      oss << node_ids[i];
    }
    oss << "\nedge_ids=";
    for (std::size_t i = 0; i < edge_ids.size(); ++i) {
      if (i != 0) {
        oss << ",";
      }
      oss << edge_ids[i];
    }
    oss << "\nsteps_attempted=" << steps_attempted << "\n";
    oss << "steps_completed=" << steps_completed << "\n";
    oss << "skipped_batches=" << skipped_batches << "\n";
    oss << "optimizer_steps=" << optimizer_steps << "\n";
    oss << "last_loss=" << last_loss << "\n";
    oss << "mean_loss=" << mean_loss << "\n";
    oss << "last_invariance_loss=" << last_invariance_loss << "\n";
    oss << "mean_invariance_loss=" << mean_invariance_loss << "\n";
    oss << "last_variance_loss=" << last_variance_loss << "\n";
    oss << "mean_variance_loss=" << mean_variance_loss << "\n";
    oss << "last_covariance_loss=" << last_covariance_loss << "\n";
    oss << "mean_covariance_loss=" << mean_covariance_loss << "\n";
    oss << "last_grad_norm=" << last_grad_norm << "\n";
    oss << "max_grad_norm=" << max_grad_norm << "\n";
    oss << "last_valid_projection_rows=" << last_valid_projection_rows << "\n";
    oss << "total_valid_projection_rows=" << total_valid_projection_rows
        << "\n";
    oss << "adapter_original_rows=" << adapter_original_rows << "\n";
    oss << "adapter_kept_rows=" << adapter_kept_rows << "\n";
    oss << "adapter_dropped_rows=" << adapter_dropped_rows << "\n";
    oss << "adapter_valid_channel_time_count="
        << adapter_valid_channel_time_count << "\n";
    oss << "adapter_kept_valid_channel_time_count="
        << adapter_kept_valid_channel_time_count << "\n";
    oss << "mean_adapter_valid_channel_time_fraction="
        << mean_adapter_valid_channel_time_fraction << "\n";
    oss << "mean_adapter_kept_valid_channel_time_fraction="
        << mean_adapter_kept_valid_channel_time_fraction << "\n";
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
    oss << "runtime_lls_emitted=" << (runtime_lls_emitted ? "true" : "false")
        << "\n";
    return oss.str();
  }
};

namespace graph_first_representation_launcher_detail {

inline double scalar_or_nan(const torch::Tensor &tensor) {
  if (!tensor.defined() || tensor.numel() == 0) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return tensor.detach()
      .to(torch::kCPU)
      .to(torch::kFloat64)
      .template item<double>();
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

inline void append_anchor_domain_warning_token(std::string &warnings,
                                               const std::string &token) {
  if (token.empty()) {
    return;
  }
  if (!warnings.empty()) {
    warnings.push_back('|');
  }
  warnings.append(token);
}

inline void populate_anchor_domain_warning(
    int64_t accepted_anchor_count, int64_t candidate_anchor_count,
    int64_t skipped_missing_edge_coverage, int64_t skipped_failed_fetch_probe,
    double accepted_anchor_fraction, std::string &warning_level,
    std::string &warnings) {
  warning_level = "none";
  warnings.clear();
  if (accepted_anchor_count == 0) {
    warning_level = "error";
    append_anchor_domain_warning_token(warnings, "accepted_anchor_count_zero");
    return;
  }
  if (std::isfinite(accepted_anchor_fraction) &&
      accepted_anchor_fraction < 0.80) {
    warning_level = "warning";
    append_anchor_domain_warning_token(warnings,
                                       "accepted_anchor_fraction_below_0.80");
  }
  if (skipped_failed_fetch_probe > 0) {
    warning_level = "warning";
    append_anchor_domain_warning_token(warnings,
                                       "skipped_failed_fetch_probe_nonzero");
  }
  const double missing_edge_fraction =
      candidate_anchor_count == 0
          ? 0.0
          : static_cast<double>(skipped_missing_edge_coverage) /
                static_cast<double>(candidate_anchor_count);
  if (missing_edge_fraction > 0.10) {
    warning_level = "warning";
    append_anchor_domain_warning_token(
        warnings, "skipped_missing_edge_coverage_fraction_above_0.10");
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
        std::string("[graph_first_representation_launcher] failed to create ") +
        path_kind + " parent directory '" + parent.string() +
        "': " + ec.message());
  }
}

inline void
write_report_file(const std::filesystem::path &path,
                  const graph_first_representation_training_report_t &report) {
  if (path.empty()) {
    throw std::runtime_error(
        "[graph_first_representation_launcher] report path is empty");
  }
  ensure_parent_directory(path, "report");
  std::ofstream out(path, std::ios::trunc);
  if (!out) {
    throw std::runtime_error(
        "[graph_first_representation_launcher] failed to open report path");
  }
  out << report.to_text();
}

inline void write_text_file(const std::filesystem::path &path,
                            const std::string &payload, const char *path_kind) {
  if (path.empty() || payload.empty()) {
    return;
  }
  ensure_parent_directory(path, path_kind);
  std::ofstream out(path, std::ios::trunc);
  if (!out) {
    throw std::runtime_error(
        std::string("[graph_first_representation_launcher] failed to open ") +
        path_kind + " path");
  }
  out << payload;
}

[[nodiscard]] inline std::vector<int64_t>
string_to_bytes(const std::string &value) {
  std::vector<int64_t> bytes;
  bytes.reserve(value.size());
  for (const unsigned char ch : value) {
    bytes.push_back(static_cast<int64_t>(ch));
  }
  return bytes;
}

[[nodiscard]] inline std::pair<std::vector<int64_t>, std::vector<int64_t>>
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

[[nodiscard]] inline torch::Tensor
int64_tensor_from_vector(const std::vector<int64_t> &values) {
  const auto opts = torch::TensorOptions().dtype(torch::kInt64);
  if (values.empty()) {
    return torch::empty({0}, opts);
  }
  return torch::tensor(values, opts);
}

inline void save_vicreg_checkpoint_file(
    const std::filesystem::path &path,
    const graph_first_representation_training_report_t &report,
    cuwacunu::wikimyei::representation::encoding::vicreg::
        vicreg_node_train_model_t &model) {
  if (path.empty()) {
    return;
  }
  ensure_parent_directory(path, "checkpoint");
  torch::serialize::OutputArchive root;
  torch::serialize::OutputArchive encoder_archive;
  model.encoder()->save(encoder_archive);
  root.write("encoder", encoder_archive);
  torch::serialize::OutputArchive optimizer_archive;
  model.optimizer().save(optimizer_archive);
  root.write("optimizer", optimizer_archive);

  const auto i64 = torch::TensorOptions().dtype(torch::kInt64);
  root.write("meta/encoding_dim", torch::tensor({report.encoding_dim}, i64));
  root.write("meta/optimizer_steps",
             torch::tensor({report.optimizer_steps}, i64));
  root.write("meta/required_feature_coords",
             int64_tensor_from_vector(report.required_feature_coords));
  root.write("meta/graph_order_fingerprint_bytes",
             int64_tensor_from_vector(
                 string_to_bytes(report.graph_order_fingerprint)));
  auto [node_id_lengths, node_id_bytes] =
      string_list_to_lengths_and_bytes(report.node_ids);
  root.write("meta/node_id_lengths", int64_tensor_from_vector(node_id_lengths));
  root.write("meta/node_id_bytes", int64_tensor_from_vector(node_id_bytes));
  root.save_to(path.string());
}

template <typename KeyT>
inline std::string make_representation_training_runtime_lls(
    const cuwacunu::wikimyei::expression::nodelift::srl::stream::
        node_lifted_batch_t<KeyT> &lifted,
    const cuwacunu::wikimyei::representation::encoding::vicreg::
        node_encoder_input_t &input,
    const cuwacunu::wikimyei::representation::encoding::vicreg::
        vicreg_node_train_step_result_t &step,
    const std::string &component_id, const std::string &assembly_token,
    const std::string &dock_binding_token,
    const cuwacunu::kikijyeba::protocol::component_stream_wave_t &stream_wave,
    cuwacunu::kikijyeba::lattice::runtime_report::runtime_report_mode_t
        runtime_report_mode,
    int64_t optimizer_steps, int64_t wave_pulse_index) {
  namespace lls = cuwacunu::kikijyeba::lattice::runtime_report;
  auto stream_report =
      cuwacunu::kikijyeba::protocol::make_component_stream_report(
          cuwacunu::kikijyeba::protocol::component_stream_identity_t{
              .component_family = "wikimyei.representation.encoding.vicreg",
              .component_id = component_id,
              .assembly_token = assembly_token,
              .dock_binding_token = dock_binding_token,
              .graph_order_fingerprint = lifted.graph_order_fingerprint,
          },
          cuwacunu::kikijyeba::protocol::make_component_stream_cursor(
              stream_wave, lifted.cursor),
          runtime_report_mode, std::numeric_limits<double>::quiet_NaN(),
          static_cast<std::uint64_t>(input.data.numel()),
          static_cast<std::uint64_t>(step.valid_projection_rows),
          step.skipped ? 1 : 0);
  auto document =
      cuwacunu::kikijyeba::protocol::make_component_stream_runtime_document(
          "wikimyei.representation.vicreg.training.runtime.v1", stream_report);
  lls::append_graph_anchor_cursor_entries(document, lifted.cursor, "batch",
                                          false);
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "optimizer_steps", static_cast<std::uint64_t>(optimizer_steps)));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "wave_pulse_index", static_cast<std::uint64_t>(wave_pulse_index)));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "original_rows",
      static_cast<std::uint64_t>(input.diagnostics.original_rows)));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "kept_rows", static_cast<std::uint64_t>(input.diagnostics.kept_rows)));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "dropped_rows",
      static_cast<std::uint64_t>(input.diagnostics.dropped_rows)));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "valid_projection_rows",
      static_cast<std::uint64_t>(step.valid_projection_rows)));
  document.entries.push_back(
      lls::make_component_runtime_lls_bool_entry("trained", !step.skipped));
  document.entries.push_back(lls::make_component_runtime_lls_bool_entry(
      "optimizer_step_applied", step.optimizer_step_applied));
  const auto add_loss = [&](const char *key, const torch::Tensor &value) {
    const double scalar = scalar_or_nan(value);
    if (std::isfinite(scalar)) {
      document.entries.push_back(lls::make_component_runtime_lls_double_entry(
          key, scalar, "(-inf,+inf)"));
    }
  };
  add_loss("loss", step.loss);
  add_loss("invariance_loss", step.invariance_loss);
  add_loss("variance_loss", step.variance_loss);
  add_loss("covariance_loss", step.covariance_loss);
  document.entries.push_back(lls::make_component_runtime_lls_string_entry(
      "loss_preference", "lower_is_better"));
  if (std::isfinite(step.grad_norm)) {
    document.entries.push_back(lls::make_component_runtime_lls_double_entry(
        "grad_norm", step.grad_norm, "[0,+inf)"));
  }
  if (std::isfinite(input.diagnostics.kept_valid_channel_time_fraction)) {
    document.entries.push_back(lls::make_component_runtime_lls_double_entry(
        "kept_valid_channel_time_fraction",
        input.diagnostics.kept_valid_channel_time_fraction, "[0,1]"));
  }
  return lls::emit_component_runtime_lls_canonical(document);
}

inline void
write_runtime_lls_sidecars(const std::filesystem::path &report_path,
                           const std::string &nodelift_runtime_lls,
                           const std::string &training_runtime_lls) {
  write_text_file(std::filesystem::path(report_path.string() + ".nodelift.lls"),
                  nodelift_runtime_lls, "nodelift runtime lls");
  write_text_file(std::filesystem::path(report_path.string() +
                                        ".representation_training.lls"),
                  training_runtime_lls, "representation training runtime lls");
}

} // namespace graph_first_representation_launcher_detail

template <typename DatatypeT> class graph_first_representation_launcher_t {
public:
  using builder_t =
      cuwacunu::kikijyeba::protocol::graph_first_pipeline_builder_t<DatatypeT>;

  graph_first_representation_launcher_t(
      builder_t builder,
      graph_first_representation_launcher_options_t options = {})
      : builder_(std::move(builder)), options_(std::move(options)) {
    validate_batch_size_contract();
  }

  [[nodiscard]] graph_first_representation_training_report_t
  dry_run_report() const {
    const auto plan = builder_.dry_run_report();
    graph_first_representation_training_report_t out{};
    out.training_id = builder_.bundle().vicreg_training.training_id;
    out.config_path = builder_.bundle().config_path;
    out.component_id = builder_.bundle().vicreg.component_id;
    out.graph_order_fingerprint = plan.graph_order_fingerprint;
    out.node_ids = plan.node_ids;
    out.edge_ids = plan.edge_ids;
    out.required_feature_coords = plan.required_feature_coords;
    out.mask_profile = plan.mask_profile;
    out.encoding_dim = builder_.bundle().vicreg.encoding_dim;
    out.effective_batch_size = plan.effective_batch_size;
    out.batch_size_source = plan.batch_size_source;
    out.dtype = plan.dtype;
    out.device = plan.device;
    out.seed = builder_.bundle().vicreg_training.seed;
    out.seed_scope = builder_.options().device.is_cuda()
                         ? "torch_manual_seed_cuda_manual_seed_all"
                         : "torch_manual_seed_cpu";
    out.checkpoint_every = builder_.bundle().vicreg_training.checkpoint_every;
    out.report_every = builder_.bundle().vicreg_training.report_every;
    out.validation_every = builder_.bundle().vicreg_training.validation_every;
    out.analytics_status = plan.analytics_status;
    out.wave_id = plan.wave_id;
    out.wave_mode = plan.wave_mode;
    out.target_action = effective_train_target() ? "train" : "run";
    out.wave_source_cursor_kind = plan.wave_source_cursor_kind;
    out.wave_source_cursor_scope = plan.wave_source_cursor_scope;
    out.wave_source_range_policy = plan.wave_source_range_policy;
    out.requested_anchor_index_begin = plan.requested_anchor_index_begin;
    out.requested_anchor_index_end = plan.requested_anchor_index_end;
    out.runtime_report_mode =
        cuwacunu::kikijyeba::settings::runtime_report_mode_name(
            cuwacunu::kikijyeba::protocol::graph_first_pipeline_builder_detail::
                resolve_runtime_report_mode(builder_.bundle().wave_settings,
                                            options_.runtime_report_mode));
    out.stream_plan = plan.stream_plan.summary();
    return out;
  }

  [[nodiscard]] graph_first_representation_training_report_t run() const {
    if (options_.dry_run) {
      return dry_run_report();
    }
    const bool train_target = effective_train_target();

    const auto checkpoint_every =
        builder_.bundle().vicreg_training.checkpoint_every;
    const auto report_every = builder_.bundle().vicreg_training.report_every;
    if ((options_.write_report || (train_target && checkpoint_every > 0)) &&
        options_.report_path.empty()) {
      throw std::runtime_error(
          "[graph_first_representation_launcher] report_path is required for "
          "configured report or checkpoint output");
    }

    const auto seed_scope =
        graph_first_representation_launcher_detail::seed_torch_runtime(
            static_cast<uint64_t>(builder_.bundle().vicreg_training.seed),
            builder_.options().device);
    const auto runtime_report_mode = cuwacunu::kikijyeba::protocol::
        graph_first_pipeline_builder_detail::resolve_runtime_report_mode(
            builder_.bundle().wave_settings, options_.runtime_report_mode);

    auto source = builder_.make_graph_source();
    const auto source_cursor_report = source.cursor_report();
    auto lifted_stream = builder_.make_node_lifted_stream(std::move(source),
                                                          runtime_report_mode);
    auto encoder = builder_.make_vicreg_encoder();
    cuwacunu::wikimyei::representation::encoding::vicreg::
        vicreg_node_train_options_t train_options{};
    train_options.grad_clip_norm =
        builder_.bundle().vicreg_training.grad_clip_norm;
    auto model = cuwacunu::wikimyei::representation::encoding::vicreg::
        vicreg_node_train_model_t(
            std::move(encoder), builder_.bundle().vicreg_training.learning_rate,
            train_options);
    const auto adapter_options = cuwacunu::wikimyei::representation::encoding::
        vicreg::node_adapter_options_from_vicreg_spec(builder_.bundle().vicreg,
                                                      /*training=*/true);

    graph_first_representation_training_report_t report = dry_run_report();
    report.runtime_report_mode =
        cuwacunu::kikijyeba::settings::runtime_report_mode_name(
            runtime_report_mode);
    report.seed_scope = seed_scope;
    report.source_cursor_token = source_cursor_report.cursor_token();
    report.source_anchor_count =
        static_cast<int64_t>(source_cursor_report.accepted_anchor_count);
    report.source_candidate_anchor_count =
        static_cast<int64_t>(source_cursor_report.candidate_anchor_count);
    report.source_skipped_anchor_count =
        static_cast<int64_t>(source_cursor_report.skipped_anchor_count());
    report.source_skipped_outside_common_range =
        static_cast<int64_t>(source_cursor_report.skipped_outside_common_range);
    report.source_skipped_missing_edge_coverage = static_cast<int64_t>(
        source_cursor_report.skipped_missing_edge_coverage);
    report.source_skipped_failed_fetch_probe =
        static_cast<int64_t>(source_cursor_report.skipped_failed_fetch_probe);
    report.source_duplicate_anchor_count =
        static_cast<int64_t>(source_cursor_report.duplicate_anchor_count);
    report.source_accepted_anchor_fraction =
        source_cursor_report.candidate_anchor_count == 0
            ? std::numeric_limits<double>::quiet_NaN()
            : static_cast<double>(source_cursor_report.accepted_anchor_count) /
                  static_cast<double>(
                      source_cursor_report.candidate_anchor_count);
    graph_first_representation_launcher_detail::populate_anchor_domain_warning(
        report.source_anchor_count, report.source_candidate_anchor_count,
        report.source_skipped_missing_edge_coverage,
        report.source_skipped_failed_fetch_probe,
        report.source_accepted_anchor_fraction,
        report.source_anchor_domain_warning_level,
        report.source_anchor_domain_warnings);
    report.source_common_left_key =
        graph_first_representation_launcher_detail::optional_key_to_string(
            source_cursor_report.common_left_key);
    report.source_common_right_key =
        graph_first_representation_launcher_detail::optional_key_to_string(
            source_cursor_report.common_right_key);
    report.source_reference_left_key =
        graph_first_representation_launcher_detail::optional_key_to_string(
            source_cursor_report.reference_left_key);
    report.source_reference_right_key =
        graph_first_representation_launcher_detail::optional_key_to_string(
            source_cursor_report.reference_right_key);
    report.source_first_anchor_key =
        graph_first_representation_launcher_detail::optional_key_to_string(
            source_cursor_report.first_anchor_key());
    report.source_last_anchor_key =
        graph_first_representation_launcher_detail::optional_key_to_string(
            source_cursor_report.last_anchor_key());

    double loss_sum = 0.0;
    double invariance_sum = 0.0;
    double variance_sum = 0.0;
    double covariance_sum = 0.0;
    int64_t loss_count = 0;
    double valid_fraction_sum = 0.0;
    double kept_valid_fraction_sum = 0.0;
    int64_t diagnostics_count = 0;
    double max_grad_norm = -std::numeric_limits<double>::infinity();

    auto refresh_running_report = [&]() {
      if (loss_count > 0) {
        report.mean_loss = loss_sum / static_cast<double>(loss_count);
        report.mean_invariance_loss =
            invariance_sum / static_cast<double>(loss_count);
        report.mean_variance_loss =
            variance_sum / static_cast<double>(loss_count);
        report.mean_covariance_loss =
            covariance_sum / static_cast<double>(loss_count);
      }
      if (diagnostics_count > 0) {
        report.mean_adapter_valid_channel_time_fraction =
            valid_fraction_sum / static_cast<double>(diagnostics_count);
        report.mean_adapter_kept_valid_channel_time_fraction =
            kept_valid_fraction_sum / static_cast<double>(diagnostics_count);
      }
      if (std::isfinite(max_grad_norm)) {
        report.max_grad_norm = max_grad_norm;
      }
    };

    auto write_checkpoint = [&]() {
      refresh_running_report();
      auto checkpoint_path = options_.report_path;
      checkpoint_path += ".vicreg.pt";
      report.checkpoint_written = true;
      ++report.checkpoint_write_count;
      report.last_checkpoint_optimizer_step = report.optimizer_steps;
      report.checkpoint_path = checkpoint_path.string();
      report.checkpoint_format = "torch_archive_vicreg_encoder_v1";
      graph_first_representation_launcher_detail::save_vicreg_checkpoint_file(
          checkpoint_path, report, model);
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
      graph_first_representation_launcher_detail::write_report_file(
          options_.report_path, report);
      if (report.runtime_lls_emitted) {
        graph_first_representation_launcher_detail::write_runtime_lls_sidecars(
            options_.report_path, report.nodelift_runtime_lls,
            report.representation_training_runtime_lls);
      }
    };

    const int64_t max_steps = builder_.bundle().vicreg_training.max_steps;
    while (lifted_stream.has_next() &&
           (max_steps == 0 || report.steps_attempted < max_steps)) {
      ++report.steps_attempted;
      ++report.wave_pulses_attempted;
      auto lifted = lifted_stream.next();
      report.wave_streamed_anchor_count +=
          static_cast<int64_t>(lifted.cursor.anchor_count());
      if (report.wave_first_anchor_key.empty()) {
        report.wave_first_anchor_key =
            graph_first_representation_launcher_detail::optional_key_to_string(
                lifted.cursor.first_anchor_key());
      }
      report.wave_last_anchor_key =
          graph_first_representation_launcher_detail::optional_key_to_string(
              lifted.cursor.last_anchor_key());

      auto input = cuwacunu::wikimyei::representation::encoding::vicreg::
          make_node_encoder_input(lifted, adapter_options);
      cuwacunu::wikimyei::representation::encoding::vicreg::
          vicreg_node_train_step_result_t step{};
      if (train_target) {
        step = model.train_one_batch(input.data, input.mask,
                                     /*swa_start_iter=*/-1,
                                     /*verbose=*/false);
      } else {
        step.valid_projection_rows = input.mask.any(/*dim=*/1)
                                         .any(/*dim=*/1)
                                         .sum()
                                         .template item<int64_t>();
        if (step.valid_projection_rows <= 0) {
          step.skipped = true;
        } else {
          auto encoded = model.encode(input.data, input.mask,
                                      /*use_swa=*/false,
                                      /*detach_to_cpu=*/false);
          step.gradients_finite =
              torch::isfinite(encoded).all().template item<bool>();
          if (!step.gradients_finite) {
            step.skipped = true;
          }
        }
      }

      report.adapter_original_rows += input.diagnostics.original_rows;
      report.adapter_kept_rows += input.diagnostics.kept_rows;
      report.adapter_dropped_rows += input.diagnostics.dropped_rows;
      report.adapter_valid_channel_time_count +=
          input.diagnostics.valid_channel_time_count;
      report.adapter_kept_valid_channel_time_count +=
          input.diagnostics.kept_valid_channel_time_count;
      valid_fraction_sum += input.diagnostics.valid_channel_time_fraction;
      kept_valid_fraction_sum +=
          input.diagnostics.kept_valid_channel_time_fraction;
      ++diagnostics_count;

      const bool pulse_completed =
          train_target ? step.optimizer_step_applied : !step.skipped;
      if (pulse_completed) {
        ++report.steps_completed;
        ++report.wave_pulses_completed;
        if (step.optimizer_step_applied) {
          ++report.optimizer_steps;
          report.last_loss =
              graph_first_representation_launcher_detail::scalar_or_nan(
                  step.loss);
          report.last_invariance_loss =
              graph_first_representation_launcher_detail::scalar_or_nan(
                  step.invariance_loss);
          report.last_variance_loss =
              graph_first_representation_launcher_detail::scalar_or_nan(
                  step.variance_loss);
          report.last_covariance_loss =
              graph_first_representation_launcher_detail::scalar_or_nan(
                  step.covariance_loss);
          if (std::isfinite(report.last_loss)) {
            loss_sum += report.last_loss;
            invariance_sum += report.last_invariance_loss;
            variance_sum += report.last_variance_loss;
            covariance_sum += report.last_covariance_loss;
            ++loss_count;
          }
        }
      } else if (step.skipped) {
        ++report.skipped_batches;
        ++report.wave_pulses_skipped;
      }
      report.last_grad_norm = step.grad_norm;
      if (std::isfinite(step.grad_norm)) {
        max_grad_norm = std::max(max_grad_norm, step.grad_norm);
      }
      report.last_valid_projection_rows = step.valid_projection_rows;
      report.total_valid_projection_rows += step.valid_projection_rows;
      report.finite_parameter_check =
          (report.finite_parameter_check != 0.0 && step.gradients_finite) ? 1.0
                                                                          : 0.0;

      if (cuwacunu::kikijyeba::lattice::runtime_report::runtime_report_enabled(
              runtime_report_mode)) {
        report.nodelift_runtime_lls = lifted.runtime_lls;
        report.representation_training_runtime_lls =
            graph_first_representation_launcher_detail::
                make_representation_training_runtime_lls(
                    lifted, input, step, builder_.bundle().vicreg.component_id,
                    cuwacunu::wikimyei::assembly::make_assembly_token(
                        builder_.bundle().vicreg_assembly.family,
                        builder_.bundle().vicreg_assembly.component_id,
                        builder_.bundle().vicreg_assembly.version_token),
                    cuwacunu::kikijyeba::topology::dock_binding_token(
                        builder_.bundle().dock_binding),
                    cuwacunu::kikijyeba::protocol::
                        component_stream_wave_from_settings(
                            builder_.bundle().wave_settings),
                    runtime_report_mode, report.optimizer_steps,
                    report.wave_pulses_attempted);
        report.runtime_lls_emitted = true;
      }

      if (train_target && step.optimizer_step_applied && checkpoint_every > 0 &&
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

private:
  [[nodiscard]] bool effective_train_target() const {
    if (!options_.train_target_from_wave) {
      return options_.train_target;
    }
    return builder_.bundle().wave_settings.action ==
           cuwacunu::kikijyeba::settings::wave_action_t::train;
  }

  void validate_batch_size_contract() const {
    const auto expected =
        static_cast<std::size_t>(builder_.bundle().vicreg_training.batch_size);
    if (builder_.options().batch_size != expected) {
      throw std::runtime_error(
          "[graph_first_representation_launcher] builder batch_size must match "
          "VICReg training BATCH_SIZE");
    }
  }

  builder_t builder_;
  graph_first_representation_launcher_options_t options_{};
};

} // namespace cuwacunu::jkimyei::training::representation
