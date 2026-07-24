// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <fcntl.h>
#include <unistd.h>

#include <torch/torch.h>

#include "piaabo/digest/sha256.h"
#include "wikimyei/inference/expected_value/mdn/channel_context_mdn_train_model.h"
#include "wikimyei/inference/expected_value/mdn/per_edge_conditioned_affine_return_head.h"

namespace cuwacunu::jkimyei::training::inference {

inline constexpr std::int64_t
    kChannelMdnConditionedAffineShadowValidationBegin = 584;
inline constexpr std::int64_t kChannelMdnConditionedAffineShadowValidationEnd =
    730;
inline constexpr const char *kChannelMdnConditionedAffineShadowReportSchema =
    "wikimyei.inference.expected_value.mdn."
    "conditioned_affine_shadow_eval.v1";

struct channel_mdn_conditioned_affine_shadow_options_t {
  std::filesystem::path artifact_path{};
  std::string artifact_file_sha256{};
  std::string artifact_semantic_digest{};

  std::filesystem::path expected_representation_checkpoint_path{};
  std::string expected_representation_checkpoint_sha256{};
  std::filesystem::path expected_mdn_checkpoint_path{};
  std::string expected_mdn_checkpoint_sha256{};

  std::string expected_source_cursor_token{};
  std::filesystem::path report_path{};
  std::int64_t close_feature_coord{3};
};

struct channel_mdn_conditioned_affine_shadow_identity_t {
  std::string graph_order_fingerprint{};
  std::vector<std::string> node_ids{};
  std::vector<std::string> edge_ids{};
  std::vector<std::int64_t> target_coords{};
  std::int64_t channel_count{0};
  std::int64_t readout_feature_dim{0};
  std::int64_t requested_anchor_index_begin{-1};
  std::int64_t requested_anchor_index_end{-1};
  std::string requested_source_key_begin{};
  std::string requested_source_key_end{};
  std::filesystem::path representation_checkpoint_path{};
  std::filesystem::path mdn_checkpoint_path{};
  torch::Device device{torch::kCPU};
  std::vector<std::filesystem::path> forbidden_report_paths{};
};

namespace channel_mdn_conditioned_affine_shadow_detail {

[[noreturn]] inline void fail(const std::string &message) {
  throw std::runtime_error("[channel_mdn_conditioned_affine_shadow] " +
                           message);
}

inline void require(bool condition, const std::string &message) {
  if (!condition) {
    fail(message);
  }
}

inline void require_single_line(const std::string &value, const char *name) {
  require(!value.empty(), std::string(name) + " must not be empty");
  require(value.find('\n') == std::string::npos &&
              value.find('\r') == std::string::npos,
          std::string(name) + " must not contain line breaks");
}

inline void require_sha256(const std::string &value, const char *name) {
  require(cuwacunu::piaabo::digest::is_sha256_hex(value),
          std::string(name) + " must be a lowercase SHA-256");
}

[[nodiscard]] inline std::filesystem::path
canonical_regular_file(const std::filesystem::path &path, const char *name) {
  require(!path.empty(), std::string(name) + " path must not be empty");
  std::error_code ec;
  const auto canonical = std::filesystem::canonical(path, ec);
  require(!ec, std::string(name) +
                   " path cannot be canonicalized: " + path.string());
  require(std::filesystem::is_regular_file(canonical, ec) && !ec,
          std::string(name) +
              " path is not a regular file: " + canonical.string());
  return canonical;
}

[[nodiscard]] inline std::filesystem::path
normalized_output_path(const std::filesystem::path &path, const char *name) {
  require(!path.empty(), std::string(name) + " path must not be empty");
  std::error_code ec;
  const auto absolute = std::filesystem::absolute(path, ec).lexically_normal();
  require(!ec, std::string(name) + " path cannot be made absolute");
  const auto parent = absolute.parent_path();
  require(!parent.empty() && std::filesystem::is_directory(parent, ec) && !ec,
          std::string(name) +
              " parent directory does not exist: " + parent.string());
  const auto canonical_parent = std::filesystem::canonical(parent, ec);
  require(!ec, std::string(name) + " parent cannot be canonicalized");
  return (canonical_parent / absolute.filename()).lexically_normal();
}

[[nodiscard]] inline std::string
read_regular_file_bytes(const std::filesystem::path &path, const char *name) {
  std::ifstream input(path, std::ios::binary);
  require(input.is_open(), std::string("failed to open ") + name +
                               " for hashing: " + path.string());
  input.seekg(0, std::ios::end);
  const auto end = input.tellg();
  require(end >= 0, std::string("failed to size ") + name);
  std::string bytes(static_cast<std::size_t>(end), '\0');
  input.seekg(0, std::ios::beg);
  if (!bytes.empty()) {
    input.read(bytes.data(), static_cast<std::streamsize>(bytes.size()));
  }
  require(input.good() || input.eof(), std::string("failed to read ") + name);
  require(static_cast<std::size_t>(input.gcount()) == bytes.size() ||
              bytes.empty(),
          std::string("short read while hashing ") + name);
  return bytes;
}

[[nodiscard]] inline std::string
sha256_regular_file(const std::filesystem::path &path, const char *name) {
  const auto canonical = canonical_regular_file(path, name);
  return cuwacunu::piaabo::digest::sha256_hex(
      read_regular_file_bytes(canonical, name));
}

inline void require_same_path(const std::filesystem::path &actual,
                              const std::filesystem::path &expected,
                              const char *name) {
  require(canonical_regular_file(actual, name) ==
              canonical_regular_file(expected, name),
          std::string(name) + " path does not match the predeclared path");
}

[[nodiscard]] inline std::vector<std::int64_t>
tensor_i64_values(const torch::Tensor &value) {
  if (!value.defined() || value.numel() == 0) {
    return {};
  }
  auto flat =
      value.detach().to(torch::kCPU).to(torch::kInt64).contiguous().view({-1});
  std::vector<std::int64_t> out(static_cast<std::size_t>(flat.numel()));
  const auto accessor = flat.accessor<std::int64_t, 1>();
  for (std::int64_t i = 0; i < flat.numel(); ++i) {
    out[static_cast<std::size_t>(i)] = accessor[i];
  }
  return out;
}

inline void accumulate_i64_values(const torch::Tensor &value,
                                  std::vector<std::int64_t> &sum) {
  const auto values = tensor_i64_values(value);
  if (values.empty()) {
    return;
  }
  if (sum.empty()) {
    sum.assign(values.size(), 0);
  }
  require(sum.size() == values.size(), "metric vector size changed");
  for (std::size_t i = 0; i < values.size(); ++i) {
    sum[i] += values[i];
  }
}

[[nodiscard]] inline double safe_ratio(double numerator,
                                       std::int64_t denominator) {
  if (denominator <= 0) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return numerator / static_cast<double>(denominator);
}

[[nodiscard]] inline double population_std(std::int64_t count, double sum,
                                           double squared_sum) {
  if (count <= 0) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  const auto n = static_cast<double>(count);
  const auto mean = sum / n;
  return std::sqrt(std::max(0.0, squared_sum / n - mean * mean));
}

[[nodiscard]] inline double correlation(std::int64_t count, double x_sum,
                                        double y_sum, double x_squared_sum,
                                        double y_squared_sum,
                                        double cross_sum) {
  if (count <= 1) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  const auto n = static_cast<double>(count);
  const auto covariance = cross_sum - x_sum * y_sum / n;
  const auto x_variance = x_squared_sum - x_sum * x_sum / n;
  const auto y_variance = y_squared_sum - y_sum * y_sum / n;
  const auto denominator =
      std::sqrt(std::max(0.0, x_variance) * std::max(0.0, y_variance));
  if (!(denominator > 0.0) || !std::isfinite(denominator)) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return covariance / denominator;
}

[[nodiscard]] inline std::string
join_strings(const std::vector<std::string> &values) {
  std::ostringstream out;
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) {
      out << ',';
    }
    out << values[i];
  }
  return out.str();
}

template <typename T>
inline void append_list(std::ostringstream &out, const char *key,
                        const std::vector<T> &values) {
  out << key << '=';
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) {
      out << ',';
    }
    out << values[i];
  }
  out << '\n';
}

struct metric_sums_t {
  std::int64_t valid_count{0};
  std::int64_t direction_correct_count{0};
  double abs_error_sum{0.0};
  double squared_error_sum{0.0};
  double signed_error_sum{0.0};
  double pred_sum{0.0};
  double realized_sum{0.0};
  double pred_squared_sum{0.0};
  double realized_squared_sum{0.0};
  double cross_sum{0.0};
  double pred_min{std::numeric_limits<double>::infinity()};
  double pred_max{-std::numeric_limits<double>::infinity()};
  double realized_min{std::numeric_limits<double>::infinity()};
  double realized_max{-std::numeric_limits<double>::infinity()};
  double margin_eps{std::numeric_limits<double>::quiet_NaN()};
  std::int64_t margin_valid_count{0};
  std::int64_t margin_direction_correct_count{0};
  std::int64_t near_zero_target_count{0};
  double rank_margin_eps{std::numeric_limits<double>::quiet_NaN()};
  std::int64_t pairwise_rank_valid_count{0};
  std::int64_t pairwise_rank_correct_count{0};
  std::int64_t margin_pairwise_rank_valid_count{0};
  std::int64_t margin_pairwise_rank_correct_count{0};
  std::int64_t best_asset_valid_count{0};
  std::int64_t best_asset_correct_count{0};
  std::vector<std::int64_t> valid_count_per_edge{};
  std::vector<std::int64_t> direction_correct_count_per_edge{};
  std::vector<std::int64_t> valid_count_per_channel{};
  std::vector<std::int64_t> direction_correct_count_per_channel{};

  void accumulate(const cuwacunu::wikimyei::inference::expected_value::mdn::
                      channel_context_mdn_train_step_result_t &step) {
    if (step.direct_edge_return_readout_valid_count <= 0) {
      return;
    }
    valid_count += step.direct_edge_return_readout_valid_count;
    direction_correct_count +=
        step.direct_edge_return_readout_direction_correct_count;
    abs_error_sum += step.direct_edge_return_readout_abs_error_sum;
    squared_error_sum += step.direct_edge_return_readout_squared_error_sum;
    signed_error_sum += step.direct_edge_return_readout_signed_error_sum;
    pred_sum += step.direct_edge_return_readout_pred_sum;
    realized_sum += step.direct_edge_return_readout_realized_sum;
    pred_squared_sum += step.direct_edge_return_readout_pred_squared_sum;
    realized_squared_sum +=
        step.direct_edge_return_readout_realized_squared_sum;
    cross_sum += step.direct_edge_return_readout_cross_sum;
    pred_min = std::min(pred_min, step.direct_edge_return_readout_pred_min);
    pred_max = std::max(pred_max, step.direct_edge_return_readout_pred_max);
    realized_min =
        std::min(realized_min, step.direct_edge_return_readout_realized_min);
    realized_max =
        std::max(realized_max, step.direct_edge_return_readout_realized_max);
    margin_eps = step.direct_edge_return_readout_margin_eps;
    margin_valid_count += step.direct_edge_return_readout_margin_valid_count;
    margin_direction_correct_count +=
        step.direct_edge_return_readout_margin_direction_correct_count;
    near_zero_target_count +=
        step.direct_edge_return_readout_near_zero_target_count;
    rank_margin_eps = step.direct_edge_return_readout_rank_margin_eps;
    pairwise_rank_valid_count +=
        step.direct_edge_return_readout_pairwise_rank_valid_count;
    pairwise_rank_correct_count +=
        step.direct_edge_return_readout_pairwise_rank_correct_count;
    margin_pairwise_rank_valid_count +=
        step.direct_edge_return_readout_margin_pairwise_rank_valid_count;
    margin_pairwise_rank_correct_count +=
        step.direct_edge_return_readout_margin_pairwise_rank_correct_count;
    best_asset_valid_count +=
        step.direct_edge_return_readout_best_asset_valid_count;
    best_asset_correct_count +=
        step.direct_edge_return_readout_best_asset_correct_count;
    accumulate_i64_values(step.direct_edge_return_readout_valid_count_per_edge,
                          valid_count_per_edge);
    accumulate_i64_values(
        step.direct_edge_return_readout_direction_correct_count_per_edge,
        direction_correct_count_per_edge);
    accumulate_i64_values(
        step.direct_edge_return_readout_valid_count_per_channel,
        valid_count_per_channel);
    accumulate_i64_values(
        step.direct_edge_return_readout_direction_correct_count_per_channel,
        direction_correct_count_per_channel);
  }
};

[[nodiscard]] inline std::vector<double>
directional_accuracy_vector(const std::vector<std::int64_t> &correct,
                            const std::vector<std::int64_t> &valid) {
  require(correct.size() == valid.size(), "metric vector size mismatch");
  std::vector<double> out(valid.size(),
                          std::numeric_limits<double>::quiet_NaN());
  for (std::size_t i = 0; i < valid.size(); ++i) {
    out[i] = safe_ratio(static_cast<double>(correct[i]), valid[i]);
  }
  return out;
}

} // namespace channel_mdn_conditioned_affine_shadow_detail

class channel_mdn_conditioned_affine_shadow_t {
public:
  channel_mdn_conditioned_affine_shadow_t(
      channel_mdn_conditioned_affine_shadow_options_t options,
      channel_mdn_conditioned_affine_shadow_identity_t identity)
      : options_(std::move(options)), identity_(std::move(identity)) {
    namespace detail = channel_mdn_conditioned_affine_shadow_detail;
    namespace mdn = cuwacunu::wikimyei::inference::expected_value::mdn;

    detail::require_sha256(options_.artifact_file_sha256,
                           "artifact_file_sha256");
    detail::require_sha256(options_.artifact_semantic_digest,
                           "artifact_semantic_digest");
    detail::require_sha256(options_.expected_representation_checkpoint_sha256,
                           "expected_representation_checkpoint_sha256");
    detail::require_sha256(options_.expected_mdn_checkpoint_sha256,
                           "expected_mdn_checkpoint_sha256");
    detail::require_single_line(options_.expected_source_cursor_token,
                                "expected_source_cursor_token");
    detail::require(options_.close_feature_coord == 3,
                    "the v1 shadow seam requires raw close feature coord 3");

    artifact_path_ =
        detail::canonical_regular_file(options_.artifact_path, "artifact");
    representation_checkpoint_path_ = detail::canonical_regular_file(
        identity_.representation_checkpoint_path, "representation checkpoint");
    mdn_checkpoint_path_ = detail::canonical_regular_file(
        identity_.mdn_checkpoint_path, "MDN checkpoint");
    detail::require_same_path(identity_.representation_checkpoint_path,
                              options_.expected_representation_checkpoint_path,
                              "representation checkpoint");
    detail::require_same_path(identity_.mdn_checkpoint_path,
                              options_.expected_mdn_checkpoint_path,
                              "MDN checkpoint");

    report_path_ =
        detail::normalized_output_path(options_.report_path, "shadow report");
    detail::require(!std::filesystem::exists(report_path_),
                    "shadow report path already exists");
    const auto temporary_report_path =
        std::filesystem::path(report_path_.string() + ".tmp");
    detail::require(!std::filesystem::exists(temporary_report_path),
                    "shadow report temporary path already exists");
    std::vector<std::filesystem::path> forbidden =
        identity_.forbidden_report_paths;
    forbidden.push_back(artifact_path_);
    forbidden.push_back(representation_checkpoint_path_);
    forbidden.push_back(mdn_checkpoint_path_);
    for (const auto &path : forbidden) {
      if (path.empty()) {
        continue;
      }
      std::error_code ec;
      std::filesystem::path normalized{};
      if (std::filesystem::exists(path, ec) && !ec) {
        normalized = std::filesystem::canonical(path, ec);
      } else {
        normalized = detail::normalized_output_path(path, "forbidden report");
      }
      detail::require(!ec && normalized != report_path_,
                      "shadow report path collides with another artifact");
    }

    artifact_file_sha256_ =
        detail::sha256_regular_file(artifact_path_, "artifact");
    representation_checkpoint_sha256_ = detail::sha256_regular_file(
        representation_checkpoint_path_, "representation checkpoint");
    mdn_checkpoint_sha256_ =
        detail::sha256_regular_file(mdn_checkpoint_path_, "MDN checkpoint");
    detail::require(artifact_file_sha256_ == options_.artifact_file_sha256,
                    "artifact file SHA-256 mismatch");
    detail::require(representation_checkpoint_sha256_ ==
                        options_.expected_representation_checkpoint_sha256,
                    "representation checkpoint SHA-256 mismatch");
    detail::require(mdn_checkpoint_sha256_ ==
                        options_.expected_mdn_checkpoint_sha256,
                    "MDN checkpoint SHA-256 mismatch");

    loaded_ = mdn::load_per_edge_conditioned_affine_return_head(artifact_path_);
    const auto &metadata = loaded_.metadata;
    detail::require(metadata.semantic_tensor_digest ==
                        options_.artifact_semantic_digest,
                    "artifact semantic digest mismatch");
    detail::require(metadata.representation_checkpoint_sha256 ==
                        representation_checkpoint_sha256_,
                    "artifact representation checkpoint binding mismatch");
    detail::require(metadata.mdn_checkpoint_sha256 == mdn_checkpoint_sha256_,
                    "artifact MDN checkpoint binding mismatch");
    detail::require(
        metadata.validation_begin ==
                kChannelMdnConditionedAffineShadowValidationBegin &&
            metadata.validation_end ==
                kChannelMdnConditionedAffineShadowValidationEnd &&
            metadata.max_anchor ==
                kChannelMdnConditionedAffineShadowValidationEnd - 1,
        "artifact is not scoped to the sealed [584,730) validation interval");
    detail::require(
        identity_.requested_anchor_index_begin == metadata.validation_begin &&
            identity_.requested_anchor_index_end == metadata.validation_end,
        "runtime anchor interval must equal artifact validation "
        "interval");
    detail::require(identity_.requested_source_key_begin.empty() &&
                        identity_.requested_source_key_end.empty(),
                    "source-key ranges are not allowed by the v1 shadow "
                    "seam");
    detail::require(metadata.graph_order_fingerprint ==
                        identity_.graph_order_fingerprint,
                    "graph order fingerprint mismatch");
    detail::require(metadata.node_ids == identity_.node_ids,
                    "node order mismatch");
    detail::require(metadata.edge_ids == identity_.edge_ids,
                    "edge order mismatch");
    detail::require(metadata.channel_count == identity_.channel_count,
                    "channel count mismatch");
    detail::require(metadata.edge_count ==
                        static_cast<std::int64_t>(identity_.node_ids.size()) -
                            1,
                    "edge count does not match node order");
    detail::require(identity_.readout_feature_dim >= metadata.feature_dim,
                    "live readout width is smaller than artifact feature "
                    "prefix");
    detail::require(std::find(identity_.target_coords.begin(),
                              identity_.target_coords.end(),
                              options_.close_feature_coord) !=
                        identity_.target_coords.end(),
                    "close feature coordinate is absent from target_coords");

    loaded_.head->to(identity_.device);
    loaded_.head->eval();
    loaded_.head->validate_registered_state();
    next_anchor_index_ = metadata.validation_begin;
  }

  void validate_source_cursor(std::string_view source_cursor_token) {
    channel_mdn_conditioned_affine_shadow_detail::require(
        source_cursor_token == options_.expected_source_cursor_token,
        "source cursor token mismatch");
    source_cursor_validated_ = true;
  }

  void observe(const torch::Tensor &readout_features,
               const torch::Tensor &future, const torch::Tensor &combined_mask,
               const std::vector<std::int64_t> &target_coords,
               std::int64_t begin_anchor_index, std::int64_t anchor_count) {
    namespace detail = channel_mdn_conditioned_affine_shadow_detail;
    namespace mdn = cuwacunu::wikimyei::inference::expected_value::mdn;
    detail::require(source_cursor_validated_,
                    "source cursor must be validated before observation");
    detail::require(anchor_count > 0 &&
                        anchor_count == readout_features.size(0),
                    "batch anchor count does not match readout batch");
    detail::require(begin_anchor_index == next_anchor_index_,
                    "shadow batches are not contiguous");
    detail::require(begin_anchor_index + anchor_count <=
                        kChannelMdnConditionedAffineShadowValidationEnd,
                    "shadow batch exceeds the validation interval");
    detail::require(target_coords == identity_.target_coords,
                    "batch target coordinates changed");
    detail::require(
        readout_features.defined() && readout_features.dim() == 4 &&
            readout_features.size(1) == loaded_.metadata.edge_count &&
            readout_features.size(2) == loaded_.metadata.channel_count &&
            readout_features.size(3) == identity_.readout_feature_dim,
        "live readout tensor identity mismatch");

    torch::NoGradGuard no_grad;
    auto prediction = loaded_.head->forward_readout_features(readout_features);
    mdn::MdnOut shadow_out{};
    shadow_out.direct_edge_return = std::move(prediction);
    mdn::channel_context_mdn_train_step_result_t shadow_step{};
    mdn::channel_context_mdn_train_detail::
        accumulate_direct_edge_return_readout_metrics(
            shadow_step, shadow_out, future, combined_mask, target_coords);
    metrics_.accumulate(shadow_step);
    ++observed_batch_count_;
    observed_anchor_count_ += anchor_count;
    next_anchor_index_ += anchor_count;
  }

  [[nodiscard]] std::string
  report_text(std::int64_t canonical_steps_attempted,
              std::int64_t canonical_steps_completed,
              std::int64_t canonical_streamed_anchor_count) const {
    namespace detail = channel_mdn_conditioned_affine_shadow_detail;
    detail::require(source_cursor_validated_,
                    "source cursor was not validated");
    detail::require(
        next_anchor_index_ == kChannelMdnConditionedAffineShadowValidationEnd &&
            observed_anchor_count_ ==
                kChannelMdnConditionedAffineShadowValidationEnd -
                    kChannelMdnConditionedAffineShadowValidationBegin,
        "shadow did not cover the complete validation interval");
    detail::require(canonical_steps_attempted == observed_batch_count_ &&
                        canonical_steps_completed == observed_batch_count_,
                    "canonical and shadow batch completion counts differ");
    detail::require(canonical_streamed_anchor_count == observed_anchor_count_,
                    "canonical and shadow streamed anchor counts differ");
    detail::require(metrics_.valid_count > 0,
                    "shadow validation produced no valid edge targets");
    const auto expected_valid_count = observed_anchor_count_ *
                                      loaded_.metadata.edge_count *
                                      loaded_.metadata.channel_count;
    const auto expected_pairwise_valid_count =
        observed_anchor_count_ * loaded_.metadata.channel_count *
        loaded_.metadata.edge_count * (loaded_.metadata.edge_count - 1) / 2;
    const auto expected_per_edge_count =
        observed_anchor_count_ * loaded_.metadata.channel_count;
    const auto expected_per_channel_count =
        observed_anchor_count_ * loaded_.metadata.edge_count;
    detail::require(metrics_.valid_count == expected_valid_count &&
                        metrics_.pairwise_rank_valid_count ==
                            expected_pairwise_valid_count &&
                        metrics_.best_asset_valid_count ==
                            observed_anchor_count_ *
                                loaded_.metadata.channel_count,
                    "shadow validation target coverage is incomplete");
    detail::require(
        metrics_.valid_count_per_edge.size() ==
                static_cast<std::size_t>(loaded_.metadata.edge_count) &&
            metrics_.valid_count_per_channel.size() ==
                static_cast<std::size_t>(loaded_.metadata.channel_count) &&
            std::all_of(metrics_.valid_count_per_edge.begin(),
                        metrics_.valid_count_per_edge.end(),
                        [&](std::int64_t count) {
                          return count == expected_per_edge_count;
                        }) &&
            std::all_of(metrics_.valid_count_per_channel.begin(),
                        metrics_.valid_count_per_channel.end(),
                        [&](std::int64_t count) {
                          return count == expected_per_channel_count;
                        }),
        "shadow validation per-edge/channel coverage is incomplete");

    const auto pred_std = detail::population_std(
        metrics_.valid_count, metrics_.pred_sum, metrics_.pred_squared_sum);
    const auto realized_std =
        detail::population_std(metrics_.valid_count, metrics_.realized_sum,
                               metrics_.realized_squared_sum);
    const auto per_edge_direction = detail::directional_accuracy_vector(
        metrics_.direction_correct_count_per_edge,
        metrics_.valid_count_per_edge);
    const auto per_channel_direction = detail::directional_accuracy_vector(
        metrics_.direction_correct_count_per_channel,
        metrics_.valid_count_per_channel);

    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::setprecision(17);
    out << "schema_id=" << kChannelMdnConditionedAffineShadowReportSchema
        << '\n';
    out << "status=complete\n";
    out << "diagnostic_only=true\n";
    out << "run_only=true\n";
    out << "policy_eligible=false\n";
    out << "canonical_output_mutated=false\n";
    out << "inference_batch_observer_exposed=false\n";
    out << "replay_or_policy_output_exposed=false\n";
    out << "artifact.path=" << artifact_path_.string() << '\n';
    out << "artifact.file_sha256=" << artifact_file_sha256_ << '\n';
    out << "artifact.semantic_tensor_digest="
        << loaded_.metadata.semantic_tensor_digest << '\n';
    out << "artifact.feature_dim=" << loaded_.metadata.feature_dim << '\n';
    out << "runtime.readout_feature_dim=" << identity_.readout_feature_dim
        << '\n';
    out << "runtime.channel_count=" << identity_.channel_count << '\n';
    out << "runtime.graph_order_fingerprint="
        << identity_.graph_order_fingerprint << '\n';
    out << "runtime.node_ids=" << detail::join_strings(identity_.node_ids)
        << '\n';
    out << "runtime.edge_ids=" << detail::join_strings(identity_.edge_ids)
        << '\n';
    out << "runtime.target_coords=";
    for (std::size_t i = 0; i < identity_.target_coords.size(); ++i) {
      if (i != 0) {
        out << ',';
      }
      out << identity_.target_coords[i];
    }
    out << '\n';
    out << "runtime.close_feature_coord=" << options_.close_feature_coord
        << '\n';
    out << "runtime.source_cursor_token="
        << options_.expected_source_cursor_token << '\n';
    out << "representation_checkpoint.path="
        << representation_checkpoint_path_.string() << '\n';
    out << "representation_checkpoint.sha256="
        << representation_checkpoint_sha256_ << '\n';
    out << "mdn_checkpoint.path=" << mdn_checkpoint_path_.string() << '\n';
    out << "mdn_checkpoint.sha256=" << mdn_checkpoint_sha256_ << '\n';
    out << "artifact.fit_begin=" << loaded_.metadata.fit_begin << '\n';
    out << "artifact.fit_end=" << loaded_.metadata.fit_end << '\n';
    out << "artifact.purge_begin=" << loaded_.metadata.purge_begin << '\n';
    out << "artifact.purge_end=" << loaded_.metadata.purge_end << '\n';
    out << "artifact.validation_begin=" << loaded_.metadata.validation_begin
        << '\n';
    out << "artifact.validation_end=" << loaded_.metadata.validation_end
        << '\n';
    out << "artifact.max_anchor=" << loaded_.metadata.max_anchor << '\n';
    out << "execution.observed_batch_count=" << observed_batch_count_ << '\n';
    out << "execution.observed_anchor_count=" << observed_anchor_count_ << '\n';
    out << "execution.canonical_steps_attempted=" << canonical_steps_attempted
        << '\n';
    out << "execution.canonical_steps_completed=" << canonical_steps_completed
        << '\n';
    out << "metrics.valid_count=" << metrics_.valid_count << '\n';
    out << "metrics.mae="
        << detail::safe_ratio(metrics_.abs_error_sum, metrics_.valid_count)
        << '\n';
    out << "metrics.rmse="
        << std::sqrt(detail::safe_ratio(metrics_.squared_error_sum,
                                        metrics_.valid_count))
        << '\n';
    out << "metrics.signed_error="
        << detail::safe_ratio(metrics_.signed_error_sum, metrics_.valid_count)
        << '\n';
    out << "metrics.directional_accuracy="
        << detail::safe_ratio(
               static_cast<double>(metrics_.direction_correct_count),
               metrics_.valid_count)
        << '\n';
    out << "metrics.correlation="
        << detail::correlation(metrics_.valid_count, metrics_.pred_sum,
                               metrics_.realized_sum, metrics_.pred_squared_sum,
                               metrics_.realized_squared_sum,
                               metrics_.cross_sum)
        << '\n';
    out << "metrics.margin_eps=" << metrics_.margin_eps << '\n';
    out << "metrics.margin_valid_count=" << metrics_.margin_valid_count << '\n';
    out << "metrics.margin_directional_accuracy="
        << detail::safe_ratio(
               static_cast<double>(metrics_.margin_direction_correct_count),
               metrics_.margin_valid_count)
        << '\n';
    out << "metrics.near_zero_target_count=" << metrics_.near_zero_target_count
        << '\n';
    out << "metrics.rank_margin_eps=" << metrics_.rank_margin_eps << '\n';
    out << "metrics.pairwise_rank_valid_count="
        << metrics_.pairwise_rank_valid_count << '\n';
    out << "metrics.pairwise_rank_accuracy="
        << detail::safe_ratio(
               static_cast<double>(metrics_.pairwise_rank_correct_count),
               metrics_.pairwise_rank_valid_count)
        << '\n';
    out << "metrics.margin_pairwise_rank_valid_count="
        << metrics_.margin_pairwise_rank_valid_count << '\n';
    out << "metrics.margin_pairwise_rank_accuracy="
        << detail::safe_ratio(
               static_cast<double>(metrics_.margin_pairwise_rank_correct_count),
               metrics_.margin_pairwise_rank_valid_count)
        << '\n';
    out << "metrics.best_asset_valid_count=" << metrics_.best_asset_valid_count
        << '\n';
    out << "metrics.best_asset_agreement="
        << detail::safe_ratio(
               static_cast<double>(metrics_.best_asset_correct_count),
               metrics_.best_asset_valid_count)
        << '\n';
    out << "metrics.prediction_mean="
        << detail::safe_ratio(metrics_.pred_sum, metrics_.valid_count) << '\n';
    out << "metrics.prediction_std=" << pred_std << '\n';
    out << "metrics.prediction_min=" << metrics_.pred_min << '\n';
    out << "metrics.prediction_max=" << metrics_.pred_max << '\n';
    out << "metrics.realized_mean="
        << detail::safe_ratio(metrics_.realized_sum, metrics_.valid_count)
        << '\n';
    out << "metrics.realized_std=" << realized_std << '\n';
    out << "metrics.realized_min=" << metrics_.realized_min << '\n';
    out << "metrics.realized_max=" << metrics_.realized_max << '\n';
    out << "metrics.prediction_to_realized_std_ratio="
        << ((std::isfinite(realized_std) && realized_std > 0.0)
                ? pred_std / realized_std
                : std::numeric_limits<double>::quiet_NaN())
        << '\n';
    detail::append_list(out, "metrics.valid_count_per_edge",
                        metrics_.valid_count_per_edge);
    detail::append_list(out, "metrics.directional_accuracy_per_edge",
                        per_edge_direction);
    detail::append_list(out, "metrics.valid_count_per_channel",
                        metrics_.valid_count_per_channel);
    detail::append_list(out, "metrics.directional_accuracy_per_channel",
                        per_channel_direction);
    return out.str();
  }

  void write_report_atomic(std::int64_t canonical_steps_attempted,
                           std::int64_t canonical_steps_completed,
                           std::int64_t canonical_streamed_anchor_count) const {
    namespace detail = channel_mdn_conditioned_affine_shadow_detail;
    const auto payload =
        report_text(canonical_steps_attempted, canonical_steps_completed,
                    canonical_streamed_anchor_count);
    const auto temporary =
        std::filesystem::path(report_path_.string() + ".tmp");
    detail::require(!std::filesystem::exists(report_path_) &&
                        !std::filesystem::exists(temporary),
                    "shadow report output is no longer exclusive");
    const int descriptor =
        ::open(temporary.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600);
    detail::require(descriptor >= 0,
                    "failed to exclusively create temporary shadow report");
    bool temporary_open = true;
    auto cleanup_temporary = [&]() {
      if (temporary_open) {
        (void)::close(descriptor);
        temporary_open = false;
      }
      std::error_code cleanup_ec;
      std::filesystem::remove(temporary, cleanup_ec);
    };
    std::size_t offset = 0;
    while (offset < payload.size()) {
      const auto written =
          ::write(descriptor, payload.data() + offset, payload.size() - offset);
      if (written < 0 && errno == EINTR) {
        continue;
      }
      if (written <= 0) {
        cleanup_temporary();
        detail::fail("failed to write temporary shadow report");
      }
      offset += static_cast<std::size_t>(written);
    }
    const bool flush_ok = ::fsync(descriptor) == 0;
    const bool close_ok = ::close(descriptor) == 0;
    temporary_open = false;
    if (!flush_ok || !close_ok) {
      cleanup_temporary();
      detail::fail("failed to flush temporary shadow report");
    }
    std::error_code ec;
    std::filesystem::create_hard_link(temporary, report_path_, ec);
    if (ec) {
      cleanup_temporary();
      detail::fail("failed to exclusively install shadow report: " +
                   ec.message());
    }
    std::filesystem::remove(temporary, ec);
    detail::require(!ec, "failed to remove installed shadow report temporary");
  }

  [[nodiscard]] const std::filesystem::path &report_path() const {
    return report_path_;
  }

private:
  channel_mdn_conditioned_affine_shadow_options_t options_{};
  channel_mdn_conditioned_affine_shadow_identity_t identity_{};
  cuwacunu::wikimyei::inference::expected_value::mdn::
      LoadedPerEdgeConditionedAffineReturnHead loaded_{};
  std::filesystem::path artifact_path_{};
  std::filesystem::path representation_checkpoint_path_{};
  std::filesystem::path mdn_checkpoint_path_{};
  std::filesystem::path report_path_{};
  std::string artifact_file_sha256_{};
  std::string representation_checkpoint_sha256_{};
  std::string mdn_checkpoint_sha256_{};
  channel_mdn_conditioned_affine_shadow_detail::metric_sums_t metrics_{};
  bool source_cursor_validated_{false};
  std::int64_t next_anchor_index_{
      kChannelMdnConditionedAffineShadowValidationBegin};
  std::int64_t observed_batch_count_{0};
  std::int64_t observed_anchor_count_{0};
};

} // namespace cuwacunu::jkimyei::training::inference
