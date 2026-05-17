// SPDX-License-Identifier: MIT
#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

#include <torch/torch.h>

#include "wikimyei/representation/encoding/vicreg/node_stream_adapter.h"

namespace cuwacunu::jkimyei::training::representation {

struct vicreg_node_stream_trainer_options_t {
  cuwacunu::wikimyei::representation::encoding::vicreg::
      node_vicreg_adapter_options_t adapter_options{
          cuwacunu::wikimyei::representation::encoding::vicreg::
              node_vicreg_training_adapter_options()};
  std::size_t max_batches{0}; // 0 means consume until stream exhaustion.
  int swa_start_iter{-1};
  bool verbose{false};
  bool skip_empty_batches{true};
};

struct vicreg_node_stream_train_batch_report_t {
  std::size_t stream_batch_index{0};
  cuwacunu::wikimyei::representation::encoding::vicreg::
      node_encoder_input_diagnostics_t adapter_diagnostics{};
  bool trained{false};
  bool skipped_empty{false};
  bool model_skipped{false};
  bool optimizer_step_applied{false};
  double loss{std::numeric_limits<double>::quiet_NaN()};
};

struct vicreg_node_stream_train_summary_t {
  std::size_t stream_batches_seen{0};
  std::size_t train_batches_applied{0};
  std::size_t empty_batches_skipped{0};
  std::size_t model_batches_skipped{0};
  std::size_t optimizer_steps_applied{0};

  int64_t original_rows{0};
  int64_t kept_rows{0};
  int64_t dropped_rows{0};
  int64_t valid_channel_time_count{0};
  int64_t kept_valid_channel_time_count{0};

  double loss_sum{0.0};
  std::size_t finite_loss_count{0};
  std::vector<vicreg_node_stream_train_batch_report_t> batches{};

  [[nodiscard]] double mean_loss() const {
    return finite_loss_count == 0
               ? std::numeric_limits<double>::quiet_NaN()
               : loss_sum / static_cast<double>(finite_loss_count);
  }
};

namespace vicreg_node_stream_trainer_detail {

template <typename ResultT>
[[nodiscard]] bool result_model_skipped(const ResultT &result) {
  if constexpr (requires { result.skipped; }) {
    return static_cast<bool>(result.skipped);
  } else {
    return false;
  }
}

template <typename ResultT>
[[nodiscard]] bool result_optimizer_step_applied(const ResultT &result) {
  if constexpr (requires { result.optimizer_step_applied; }) {
    return static_cast<bool>(result.optimizer_step_applied);
  } else {
    return false;
  }
}

template <typename ResultT> [[nodiscard]] double result_loss(const ResultT &r) {
  if constexpr (requires { r.loss; }) {
    if (r.loss.defined() && r.loss.numel() > 0) {
      return r.loss.detach()
          .to(torch::kCPU)
          .to(torch::kFloat64)
          .template item<double>();
    }
    return std::numeric_limits<double>::quiet_NaN();
  } else {
    return std::numeric_limits<double>::quiet_NaN();
  }
}

inline void add_adapter_diagnostics(
    vicreg_node_stream_train_summary_t &summary,
    const cuwacunu::wikimyei::representation::encoding::vicreg::
        node_encoder_input_diagnostics_t &diagnostics) {
  summary.original_rows += diagnostics.original_rows;
  summary.kept_rows += diagnostics.kept_rows;
  summary.dropped_rows += diagnostics.dropped_rows;
  summary.valid_channel_time_count += diagnostics.valid_channel_time_count;
  summary.kept_valid_channel_time_count +=
      diagnostics.kept_valid_channel_time_count;
}

} // namespace vicreg_node_stream_trainer_detail

template <typename NodeLiftedStreamT, typename ModelT>
[[nodiscard]] vicreg_node_stream_train_summary_t train_vicreg_node_stream(
    NodeLiftedStreamT &stream, ModelT &model,
    const vicreg_node_stream_trainer_options_t &options = {}) {
  vicreg_node_stream_train_summary_t summary{};

  while (stream.has_next()) {
    if (options.max_batches > 0 &&
        summary.stream_batches_seen >= options.max_batches) {
      break;
    }

    const std::size_t stream_batch_index = summary.stream_batches_seen;
    auto lifted = stream.next();
    ++summary.stream_batches_seen;

    auto input = cuwacunu::wikimyei::representation::encoding::vicreg::
        make_node_encoder_input(lifted, options.adapter_options);

    vicreg_node_stream_train_batch_report_t report{};
    report.stream_batch_index = stream_batch_index;
    report.adapter_diagnostics = input.diagnostics;
    vicreg_node_stream_trainer_detail::add_adapter_diagnostics(
        summary, input.diagnostics);

    const bool empty_batch = input.data.size(0) == 0;
    if (empty_batch && options.skip_empty_batches) {
      report.skipped_empty = true;
      ++summary.empty_batches_skipped;
      summary.batches.push_back(std::move(report));
      continue;
    }

    auto result = model.train_one_batch(
        input.data, input.mask, options.swa_start_iter, options.verbose);
    report.trained = true;
    report.model_skipped =
        vicreg_node_stream_trainer_detail::result_model_skipped(result);
    report.optimizer_step_applied =
        vicreg_node_stream_trainer_detail::result_optimizer_step_applied(
            result);
    report.loss = vicreg_node_stream_trainer_detail::result_loss(result);

    ++summary.train_batches_applied;
    if (report.model_skipped) {
      ++summary.model_batches_skipped;
    }
    if (report.optimizer_step_applied) {
      ++summary.optimizer_steps_applied;
    }
    if (std::isfinite(report.loss)) {
      summary.loss_sum += report.loss;
      ++summary.finite_loss_count;
    }
    summary.batches.push_back(std::move(report));
  }

  return summary;
}

} // namespace cuwacunu::jkimyei::training::representation
