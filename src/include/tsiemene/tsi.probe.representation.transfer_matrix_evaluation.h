// ./include/tsiemene/tsi.probe.representation.transfer_matrix_evaluation.h
// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <limits>
#include <memory>
#include <optional>
#include <numeric>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include <torch/torch.h>

#include "camahjucunu/dsl/observation_pipeline/observation_spec.h"
#include "camahjucunu/dsl/iitepi_wave/iitepi_wave.h"
#include "hashimyei/hashimyei_artifacts.h"
#include "hashimyei/hashimyei_identity.h"
#include "iitepi/contract_space_t.h"
#include "piaabo/dlogs.h"
#include "piaabo/torch_compat/torch_utils.h"
#include "tsiemene/tsi.cargo.validation.h"
#include "tsiemene/tsi.probe.representation.h"
#include "wikimyei/inference/mdn/mixture_density_network.h"
#include "wikimyei/inference/mdn/mixture_density_network_utils.h"

DEV_WARNING(
    "[tsi.probe.representation.transfer_matrix_evaluation.h] Transfer-matrix probe buffers valid epoch batches, performs epoch-end supervised training from representation cargo, emits colored matrix-evaluation summaries, and persists hashimyei-scoped metric history. Scope is task/probe extractability diagnostics only (NLL/effort/support/skill); source entropic load and network spectral-capacity margin live in dedicated data/network analytics modules.\n");

namespace tsiemene {

class TsiProbeRepresentationTransferMatrixEvaluation final : public TsiProbeRepresentation {
 public:
  using WaveProbePolicy = cuwacunu::camahjucunu::iitepi_wave_probe_policy_t;

  [[nodiscard]] static constexpr WaveProbePolicy default_wave_policy() {
    return WaveProbePolicy{
        .training_window =
            cuwacunu::camahjucunu::iitepi_wave_probe_training_window_e::
                IncomingBatch,
        .report_policy =
            cuwacunu::camahjucunu::iitepi_wave_probe_report_policy_e::
                EpochEndLog,
        .objective =
            cuwacunu::camahjucunu::iitepi_wave_probe_objective_e::
                FutureTargetDimsNll,
    };
  }

  explicit TsiProbeRepresentationTransferMatrixEvaluation(
      TsiId id,
      std::string contract_hash,
      std::string instance_name =
          "tsi.probe.representation.transfer_matrix_evaluation",
      std::string probe_hashimyei = {},
      WaveProbePolicy policy = default_wave_policy())
      : TsiProbeRepresentation(id, std::move(instance_name)),
        contract_hash_(std::move(contract_hash)),
        policy_(policy),
        probe_hashimyei_(std::move(probe_hashimyei)) {
    load_config_();
    validate_wave_policy_or_throw_();
    initialize_runtime_model_or_throw_();
    initialize_probe_hashimyei_state_();
  }

  [[nodiscard]] std::string_view type_name() const noexcept override {
    return "tsi.probe.representation.transfer_matrix_evaluation";
  }

  void step(const Wave& wave, Ingress in, BoardContext& ctx, Emitter& out) override {
    if (in.directive != IN_STEP) return;
    if (in.signal.kind != PayloadKind::Cargo) return;
    if (!ctx.debug_enabled) return;

    ++epoch_.seen;

    if (!in.signal.cargo) {
      ++epoch_.null_cargo;
      emit_meta_(wave, out, "transfer_matrix_eval.warn reason=null-cargo");
      emit_periodic_summary_(wave, out, /*force=*/false);
      return;
    }

    const ObservationCargo& sample = *in.signal.cargo;
    bool diagnostics_anomaly = false;
    bool hard_invalid = false;

    if (cfg_.validate_vicreg_out) {
      std::string cargo_error;
      if (!validate_observation_cargo(sample, CargoValidationStage::VicregOut,
                                      &cargo_error)) {
        ++epoch_.invalid_cargo;
        diagnostics_anomaly = true;
        const bool missing_future_mask_only =
            (cargo_error == "future features/mask partial") &&
            sample.future_features.defined() &&
            sample.future_features.numel() > 0 &&
            (!sample.future_mask.defined() || sample.future_mask.numel() == 0);
        hard_invalid = !missing_future_mask_only;
        emit_meta_(wave,
                   out,
                   std::string("transfer_matrix_eval.warn reason=invalid-cargo detail=") +
                       cargo_error);
      }
    }

    const bool has_encoding = sample.encoding.defined() && sample.encoding.numel() > 0;
    if (!has_encoding) {
      ++epoch_.missing_encoding;
      diagnostics_anomaly = true;
      hard_invalid = true;
      emit_meta_(wave, out, "transfer_matrix_eval.warn reason=missing-encoding");
    }

    if (has_encoding && !encoding_all_finite_(sample.encoding)) {
      ++epoch_.non_finite_encoding;
      diagnostics_anomaly = true;
      hard_invalid = true;
      emit_meta_(wave, out, "transfer_matrix_eval.warn reason=non-finite-encoding");
    }

    if (has_future_values_(sample)) {
      ++epoch_.future_values_present;
      diagnostics_anomaly = true;
      emit_meta_(wave, out, "transfer_matrix_eval.warn reason=future-values-present");
    }

    if (cfg_.check_temporal_order) {
      const auto order = temporal_order_ok_(sample);
      if (order.has_value() && !*order) {
        ++epoch_.temporal_overlap;
        diagnostics_anomaly = true;
        hard_invalid = true;
        emit_meta_(wave, out, "transfer_matrix_eval.warn reason=temporal-overlap");
      }
    }

    if (diagnostics_anomaly && cfg_.report_shapes) {
      emit_meta_(wave,
                 out,
                 std::string("transfer_matrix_eval.shape encoding=") +
                     tensor_shape_(sample.encoding) + " future_features=" +
                     tensor_shape_(sample.future_features) + " future_mask=" +
                     tensor_shape_(sample.future_mask) + " past_keys=" +
                     tensor_shape_(sample.past_keys) + " future_keys=" +
                     tensor_shape_(sample.future_keys));
    }

    if (hard_invalid) {
      emit_periodic_summary_(wave, out, /*force=*/false);
      return;
    }

    train_batch_t batch{};
    std::string prep_error;
    if (!prepare_train_batch_(sample, &batch, &prep_error)) {
      emit_meta_(wave,
                 out,
                 std::string("transfer_matrix_eval.warn reason=train-skip detail=") +
                     prep_error);
      emit_periodic_summary_(wave, out, /*force=*/false);
      return;
    }

    batch.anchor_key = derive_anchor_key_(sample, epoch_.seen);
    ++epoch_.accepted;
    buffer_epoch_batch_(std::move(batch));

    emit_periodic_summary_(wave, out, /*force=*/false);
  }

  void on_epoch_end(BoardContext& ctx) override {
    if (!ctx.debug_enabled) {
      reset_epoch_accumulators_();
      return;
    }
    run_epoch_training_cycle_();
    update_persisted_epoch_history_();
    log_info("%s\n", build_epoch_report_string_(/*beautify=*/true).c_str());
    reset_epoch_accumulators_();
  }

  void reset(BoardContext&) override {
    reset_epoch_accumulators_();
  }

 private:
  struct config_t {
    bool check_temporal_order{true};
    bool validate_vicreg_out{true};
    bool report_shapes{false};
    bool reset_hashimyei_on_start{false};

    // Probe-local simple MDN architecture knobs.
    std::int64_t mdn_mixture_comps{1};
    std::int64_t mdn_features_hidden{16};
    std::int64_t mdn_residual_depth{0};
    std::vector<std::int64_t> mdn_target_dims{};

    double optimizer_lr{1e-3};
    double optimizer_weight_decay{1e-2};
    double optimizer_beta1{0.9};
    double optimizer_beta2{0.999};
    double optimizer_eps{1e-8};
    double grad_clip{1.0};

    double nll_eps{1e-6};
    double nll_sigma_min{1e-3};
    double nll_sigma_max{0.0};

    // Anchor-split / prequential protocol knobs.
    double anchor_train_ratio{0.70};
    double anchor_val_ratio{0.15};
    double anchor_test_ratio{0.15};
    std::int64_t prequential_blocks{4};
    double linear_ridge_lambda{1e-3};
    double gaussian_var_min{1e-6};
    std::int64_t control_shuffle_block{16};
    std::int64_t control_shuffle_seed{1337};

    std::int64_t summary_every_steps{64};
  };

  struct model_spec_t {
    std::int64_t encoding_dims{0};
    std::int64_t channels{0};
    std::int64_t horizons{0};
    std::int64_t mixture_comps{0};
    std::int64_t features_hidden{0};
    std::int64_t residual_depth{0};
    std::vector<std::int64_t> target_dims{};
    torch::Dtype dtype{torch::kFloat32};
    torch::Device device{torch::kCPU};
  };

  struct epoch_stats_t {
    std::uint64_t seen{0};
    std::uint64_t accepted{0};
    std::uint64_t null_cargo{0};
    std::uint64_t invalid_cargo{0};
    std::uint64_t missing_encoding{0};
    std::uint64_t non_finite_encoding{0};
    std::uint64_t future_values_present{0};
    std::uint64_t temporal_overlap{0};
    std::uint64_t missing_future{0};
    std::uint64_t shape_mismatch{0};
    std::uint64_t optimizer_failures{0};
    std::uint64_t train_steps{0};
  };

  enum class anchor_split_e : std::uint8_t { Train = 0, Val = 1, Test = 2 };

  struct row_result_t {
    double support{std::numeric_limits<double>::quiet_NaN()};
    bool has_support{false};
    double effort{std::numeric_limits<double>::quiet_NaN()};
    bool has_effort{false};
    double error{std::numeric_limits<double>::quiet_NaN()};
    bool has_error{false};
    double effort_skill{std::numeric_limits<double>::quiet_NaN()};
    bool has_effort_skill{false};
    double error_skill{std::numeric_limits<double>::quiet_NaN()};
    bool has_error_skill{false};
    double n90{std::numeric_limits<double>::quiet_NaN()};
    bool has_n90{false};
    double selectivity{std::numeric_limits<double>::quiet_NaN()};
    bool has_selectivity{false};
  };

  struct matrix_results_t {
    row_result_t forecast_null{};
    row_result_t forecast_stats_only{};
    row_result_t forecast_linear{};
    row_result_t forecast_residual_linear{};
    row_result_t forecast_mdn{};
    double cold_start_loss{std::numeric_limits<double>::quiet_NaN()};
    bool has_cold_start_loss{false};
    double train_fit_loss{std::numeric_limits<double>::quiet_NaN()};
    bool has_train_fit_loss{false};
    double generalization_gap{std::numeric_limits<double>::quiet_NaN()};
    bool has_generalization_gap{false};
  };

  struct train_batch_t {
    torch::Tensor encoding{};
    torch::Tensor stats{};
    torch::Tensor target{};
    torch::Tensor future_mask{};
    std::string anchor_key{};
  };

  struct epoch_report_metrics_t {
    std::uint64_t invalid{0};
    std::uint64_t buffered_batches{0};
    matrix_results_t matrix{};
  };

  struct history_metric_t {
    bool higher_is_better{false};
    bool has_value{false};
    double best{0.0};
    double worst{0.0};
    double min_seen{0.0};
    double max_seen{0.0};
    double mean{0.0};
    double m2{0.0};
    std::uint64_t updates{0};
  };

  struct report_cell_t {
    std::string key{};
    const char* label{""};
    double value{std::numeric_limits<double>::quiet_NaN()};
    bool valid{false};
    bool higher_is_better{false};
    bool integer_style{false};
  };

  struct flat_dataset_t {
    torch::Tensor x{};  // [N,De]
    torch::Tensor y{};  // [N,Dy]
    torch::Tensor w{};  // [N]
  };

  struct gaussian_baseline_t {
    torch::Tensor mu{};  // [Dy]
    double sigma2{0.0};
    bool valid{false};
  };

  struct linear_gaussian_t {
    torch::Tensor beta{};  // [De+1,Dy] (last row is bias)
    double sigma2{0.0};
    bool valid{false};
  };

  struct target_norm_t {
    torch::Tensor mean{};  // [Dy] (CPU/Float64)
    torch::Tensor std{};   // [Dy] (CPU/Float64)
    double logdet_bits{0.0};
    bool valid{false};
  };

  enum class feature_mode_e : std::uint8_t { Encoding = 0, Stats = 1 };

  [[nodiscard]] static std::string tensor_shape_(const torch::Tensor& t) {
    if (!t.defined()) return "undefined";
    std::ostringstream oss;
    oss << "[";
    const auto sizes = t.sizes();
    for (std::size_t i = 0; i < sizes.size(); ++i) {
      if (i > 0) oss << ",";
      oss << sizes[i];
    }
    oss << "]";
    return oss.str();
  }

  [[nodiscard]] static std::string tensor_signature_(const torch::Tensor& t) {
    if (!t.defined() || t.numel() == 0) return "undef";
    try {
      auto cpu = t.detach().to(torch::kCPU).to(torch::kFloat64).flatten();
      const auto n = cpu.numel();
      const double first = cpu[0].item<double>();
      const double last = cpu[n - 1].item<double>();
      const double mean = cpu.mean().item<double>();
      std::ostringstream oss;
      oss << "n=" << n
          << ",f=" << std::setprecision(17) << first
          << ",l=" << std::setprecision(17) << last
          << ",m=" << std::setprecision(17) << mean;
      return oss.str();
    } catch (...) {
      return "sig_error";
    }
  }

  [[nodiscard]] static std::string derive_anchor_key_(const ObservationCargo& sample,
                                                       std::uint64_t seen_index) {
    std::ostringstream oss;
    if (sample.future_keys.defined() && sample.future_keys.numel() > 0) {
      oss << "future_keys{" << tensor_signature_(sample.future_keys) << "}";
    } else if (sample.past_keys.defined() && sample.past_keys.numel() > 0) {
      oss << "past_keys{" << tensor_signature_(sample.past_keys) << "}";
    } else {
      oss << "seen=" << seen_index;
    }
    oss << "|enc=" << tensor_shape_(sample.encoding)
        << "|future=" << tensor_shape_(sample.future_features);
    return oss.str();
  }

  [[nodiscard]] static bool encoding_all_finite_(const torch::Tensor& t) {
    if (!t.defined() || t.numel() == 0) return true;
    return torch::isfinite(t).all().item<bool>();
  }

  [[nodiscard]] static bool has_future_values_(const ObservationCargo& sample) {
    if (sample.future_mask.defined() && sample.future_mask.numel() > 0) {
      torch::Tensor m = sample.future_mask;
      if (m.dtype() != torch::kBool) m = m.ne(0);
      return m.any().item<bool>();
    }
    return sample.future_features.defined() && sample.future_features.numel() > 0;
  }

  [[nodiscard]] static std::optional<bool> temporal_order_ok_(
      const ObservationCargo& sample) {
    auto to_bcl = [](const torch::Tensor& t) -> std::optional<torch::Tensor> {
      if (!t.defined() || t.numel() == 0) return std::nullopt;
      if (t.dim() == 1) return t.view({1, 1, t.size(0)});
      if (t.dim() == 2) return t.unsqueeze(0);
      if (t.dim() == 3) return t;
      return std::nullopt;
    };

    const auto past_opt = to_bcl(sample.past_keys);
    const auto future_opt = to_bcl(sample.future_keys);
    if (!past_opt.has_value() || !future_opt.has_value()) return std::nullopt;

    torch::Tensor past = *past_opt;
    torch::Tensor future = *future_opt;
    if (past.size(2) <= 0 || future.size(2) <= 0) return std::nullopt;

    torch::Tensor past_mask;
    if (sample.mask.defined() && sample.mask.numel() > 0) {
      const auto past_mask_opt = to_bcl(sample.mask);
      if (!past_mask_opt.has_value()) return std::nullopt;
      past_mask = past_mask_opt->ne(0);
      if (past_mask.size(2) != past.size(2)) return std::nullopt;
    } else {
      past_mask = torch::ones_like(past, torch::kBool);
    }

    torch::Tensor future_mask;
    if (sample.future_mask.defined() && sample.future_mask.numel() > 0) {
      const auto future_mask_opt = to_bcl(sample.future_mask);
      if (!future_mask_opt.has_value()) return std::nullopt;
      future_mask = future_mask_opt->ne(0);
      if (future_mask.size(2) != future.size(2)) return std::nullopt;
    } else {
      future_mask = torch::ones_like(future, torch::kBool);
    }

    const auto B = std::max(past.size(0), future.size(0));
    const auto C = std::max(past.size(1), future.size(1));

    auto can_expand_to = [B, C](const torch::Tensor& t) -> bool {
      return (t.size(0) == B || t.size(0) == 1) &&
             (t.size(1) == C || t.size(1) == 1);
    };
    if (!can_expand_to(past) || !can_expand_to(future) ||
        !can_expand_to(past_mask) || !can_expand_to(future_mask)) {
      return std::nullopt;
    }

    past = past.expand({B, C, past.size(2)});
    future = future.expand({B, C, future.size(2)});
    past_mask = past_mask.expand({B, C, past_mask.size(2)});
    future_mask = future_mask.expand({B, C, future_mask.size(2)});

    const auto past_len = past.size(2);
    const auto future_len = future.size(2);

    auto past_idx = torch::arange(
        past_len,
        torch::TensorOptions().device(past.device()).dtype(torch::kLong));
    past_idx = past_idx.view({1, 1, past_len}).expand({B, C, past_len});
    auto past_last_idx =
        torch::where(past_mask, past_idx, torch::full_like(past_idx, -1))
            .amax(/*dim=*/2);
    auto past_has = past_last_idx.ge(0);

    auto future_idx = torch::arange(
        future_len,
        torch::TensorOptions().device(future.device()).dtype(torch::kLong));
    future_idx = future_idx.view({1, 1, future_len}).expand({B, C, future_len});
    auto future_first_idx =
        torch::where(future_mask,
                     future_idx,
                     torch::full_like(future_idx, future_len))
            .amin(/*dim=*/2);
    auto future_has = future_first_idx.lt(future_len);

    auto comparable = past_has.logical_and(future_has);
    if (!comparable.any().item<bool>()) return std::nullopt;

    auto past_last_idx_safe = past_last_idx.clamp_min(0).unsqueeze(-1);
    auto future_first_idx_safe = future_first_idx.clamp(0, future_len - 1).unsqueeze(-1);

    auto past_last = past.gather(/*dim=*/2, past_last_idx_safe).squeeze(-1);
    auto future_first =
        future.gather(/*dim=*/2, future_first_idx_safe).squeeze(-1);
    auto order_ok = future_first.gt(past_last);

    const bool has_violation =
        comparable.logical_and(order_ok.logical_not()).any().item<bool>();
    return !has_violation;
  }

  [[nodiscard]] torch::Tensor select_targets_(
      const torch::Tensor& future_features) {
    TORCH_CHECK(future_features.defined(),
                "[transfer_matrix_eval] future_features undefined");
    TORCH_CHECK(future_features.dim() == 4,
                "[transfer_matrix_eval] future_features must be [B,C,Hf,D]");
    const auto B = future_features.size(0);
    const auto C = future_features.size(1);
    const auto Hf = future_features.size(2);
    const auto D = future_features.size(3);

    TORCH_CHECK(!model_.target_dims.empty(),
                "[transfer_matrix_eval] target_dims is empty");
    for (const auto d : model_.target_dims) {
      TORCH_CHECK(d >= 0 && d < D,
                  "[transfer_matrix_eval] target dim out of range");
    }

    auto idx = target_index_;
    if (!idx.defined() || idx.device() != future_features.device()) {
      idx = torch::tensor(
          model_.target_dims,
          torch::TensorOptions().dtype(torch::kLong).device(torch::kCPU));
      idx = idx.to(future_features.device());
      target_index_ = idx;
    }

    const auto Dy = static_cast<std::int64_t>(model_.target_dims.size());
    auto flat = future_features.reshape({B * C * Hf, D});
    auto idx2 = idx.unsqueeze(0).expand({B * C * Hf, Dy});
    auto y = flat.gather(/*dim=*/1, idx2);
    return y.view({B, C, Hf, Dy});
  }

  [[nodiscard]] torch::Tensor build_stats_features_(const ObservationCargo& sample,
                                                    std::int64_t expected_b) {
    if (!sample.features.defined() || sample.features.numel() == 0) {
      return torch::Tensor();
    }
    if (sample.features.dim() != 4) {
      return torch::Tensor();
    }

    auto past = sample.features.to(
        model_.device, model_.dtype, /*non_blocking=*/true, /*copy=*/false);
    if (past.size(0) != expected_b || past.size(0) <= 0 || past.size(1) <= 0 ||
        past.size(2) <= 0 || past.size(3) <= 0) {
      return torch::Tensor();
    }

    std::vector<std::int64_t> dims{};
    dims.reserve(model_.target_dims.size());
    const auto D = past.size(3);
    for (const auto d : model_.target_dims) {
      if (d >= 0 && d < D) dims.push_back(d);
    }
    if (dims.empty()) return torch::Tensor();

    auto idx = torch::tensor(
        dims, torch::TensorOptions().dtype(torch::kLong).device(model_.device));
    auto selected = past.index_select(/*dim=*/3, idx);  // [B,C,T,Dy]

    torch::Tensor mask;
    if (sample.mask.defined() && sample.mask.numel() > 0 && sample.mask.dim() == 3) {
      mask = sample.mask.to(
          model_.device, torch::kFloat32, /*non_blocking=*/true, /*copy=*/false);
      if (mask.size(0) != selected.size(0) || mask.size(1) != selected.size(1) ||
          mask.size(2) != selected.size(2)) {
        mask = torch::ones(
            {selected.size(0), selected.size(1), selected.size(2)},
            torch::TensorOptions().device(model_.device).dtype(torch::kFloat32));
      } else {
        mask = mask.ne(0).to(torch::kFloat32);
      }
    } else {
      mask = torch::ones(
          {selected.size(0), selected.size(1), selected.size(2)},
          torch::TensorOptions().device(model_.device).dtype(torch::kFloat32));
    }

    auto w = mask.unsqueeze(-1);  // [B,C,T,1]
    auto mean_num = (selected * w).sum({1, 2});  // [B,Dy]
    auto mean_den = w.sum({1, 2}).clamp_min(1.0);  // [B,1]
    auto mean = mean_num / mean_den;

    const auto T = selected.size(2);
    auto last_slice = selected.select(/*dim=*/2, T - 1);  // [B,C,Dy]
    auto last_w = mask.select(/*dim=*/2, T - 1).unsqueeze(-1);  // [B,C,1]
    auto last_num = (last_slice * last_w).sum(/*dim=*/1);  // [B,Dy]
    auto last_den = last_w.sum(/*dim=*/1).clamp_min(1.0);  // [B,1]
    auto last = last_num / last_den;

    auto stats = torch::cat({mean, last}, /*dim=*/1);  // [B,2*Dy]
    stats = torch::where(torch::isfinite(stats), stats, torch::zeros_like(stats));
    return stats;
  }

  [[nodiscard]] bool prepare_train_batch_(const ObservationCargo& sample,
                                          train_batch_t* out,
                                          std::string* error) {
    if (error) error->clear();
    if (!out) return false;

    if (!sample.future_features.defined() || sample.future_features.numel() == 0) {
      ++epoch_.missing_future;
      if (error) *error = "future_features missing";
      return false;
    }
    if (sample.future_features.dim() != 4) {
      ++epoch_.shape_mismatch;
      if (error) {
        *error = "future_features must be [B,C,Hf,D], got " +
                 tensor_shape_(sample.future_features);
      }
      return false;
    }

    if (!sample.encoding.defined() || sample.encoding.numel() == 0) {
      ++epoch_.missing_encoding;
      if (error) *error = "encoding missing";
      return false;
    }
    if (sample.encoding.dim() != 2 && sample.encoding.dim() != 3) {
      ++epoch_.shape_mismatch;
      if (error) {
        *error = "encoding must be [B,De] or [B,T',De], got " +
                 tensor_shape_(sample.encoding);
      }
      return false;
    }

    torch::Tensor encoding =
        sample.encoding.to(model_.device, model_.dtype, /*non_blocking=*/true,
                           /*copy=*/false);
    if (encoding.dim() == 3) {
      encoding = encoding.mean(/*dim=*/1);
    }

    if (encoding.dim() != 2) {
      ++epoch_.shape_mismatch;
      if (error) {
        *error = "pooled encoding must be [B,De], got " + tensor_shape_(encoding);
      }
      return false;
    }

    if (encoding.size(1) != model_.encoding_dims) {
      ++epoch_.shape_mismatch;
      if (error) {
        *error = "encoding_dims mismatch expected=" +
                 std::to_string(model_.encoding_dims) + " got=" +
                 std::to_string(encoding.size(1));
      }
      return false;
    }

    torch::Tensor future = sample.future_features.to(
        model_.device, model_.dtype, /*non_blocking=*/true, /*copy=*/false);
    if (future.size(0) != encoding.size(0)) {
      ++epoch_.shape_mismatch;
      if (error) {
        *error = "batch mismatch encoding.B=" + std::to_string(encoding.size(0)) +
                 " future.B=" + std::to_string(future.size(0));
      }
      return false;
    }

    if (future.size(1) != model_.channels || future.size(2) != model_.horizons) {
      ++epoch_.shape_mismatch;
      if (error) {
        *error = "future axes mismatch expected [C,Hf]=[" +
                 std::to_string(model_.channels) + "," +
                 std::to_string(model_.horizons) + "] got [" +
                 std::to_string(future.size(1)) + "," +
                 std::to_string(future.size(2)) + "]";
      }
      return false;
    }

    torch::Tensor mask;
    if (sample.future_mask.defined() && sample.future_mask.numel() > 0) {
      if (sample.future_mask.dim() != 3) {
        ++epoch_.shape_mismatch;
        if (error) {
          *error = "future_mask must be [B,C,Hf], got " +
                   tensor_shape_(sample.future_mask);
        }
        return false;
      }
      mask = sample.future_mask.to(model_.device,
                                   torch::kFloat32,
                                   /*non_blocking=*/true,
                                   /*copy=*/false);
      if (mask.size(0) != future.size(0) || mask.size(1) != future.size(1) ||
          mask.size(2) != future.size(2)) {
        ++epoch_.shape_mismatch;
        if (error) {
          *error = "future mask shape mismatch features=" + tensor_shape_(future) +
                   " mask=" + tensor_shape_(mask);
        }
        return false;
      }
      mask = mask.ne(0).to(torch::kFloat32);
    } else {
      mask = torch::ones(
          {future.size(0), future.size(1), future.size(2)},
          torch::TensorOptions().device(model_.device).dtype(torch::kFloat32));
    }

    try {
      out->encoding = std::move(encoding);
      out->stats = build_stats_features_(sample, out->encoding.size(0));
      out->target = select_targets_(future);
      out->future_mask = std::move(mask);
    } catch (const std::exception& e) {
      ++epoch_.shape_mismatch;
      if (error) *error = std::string("target selection failed: ") + e.what();
      return false;
    }

    return true;
  }

  [[nodiscard]] torch::Tensor weighted_masked_nll_reduce_(
      const torch::Tensor& nll_map,
      const torch::Tensor& mask) {
    auto weights = mask.defined() ? mask.to(nll_map.dtype()) : torch::ones_like(nll_map);

    if (channel_weights_.defined() && channel_weights_.numel() == nll_map.size(1)) {
      auto wch = channel_weights_.to(
          nll_map.device(), nll_map.dtype(), /*non_blocking=*/true, /*copy=*/false);
      weights = weights * wch.view({1, nll_map.size(1), 1});
    }

    auto w_hz = torch::ones(
        {nll_map.size(2)},
        torch::TensorOptions().device(nll_map.device()).dtype(nll_map.dtype()));
    weights = weights * w_hz.view({1, 1, nll_map.size(2)});

    const auto denom = weights.sum().clamp_min(1.0);
    return (nll_map * weights).sum() / denom;
  }

  void buffer_epoch_batch_(train_batch_t&& batch) {
    train_batch_t stored{};
    stored.encoding =
        batch.encoding.detach().to(torch::kCPU, model_.dtype);
    if (batch.stats.defined() && batch.stats.numel() > 0) {
      stored.stats =
          batch.stats.detach().to(torch::kCPU, model_.dtype);
    }
    stored.target =
        batch.target.detach().to(torch::kCPU, model_.dtype);
    stored.future_mask =
        batch.future_mask.detach().to(torch::kCPU, torch::kFloat32);
    stored.anchor_key = std::move(batch.anchor_key);
    epoch_batches_.push_back(std::move(stored));
  }

  [[nodiscard]] bool batch_to_runtime_(const train_batch_t& stored,
                                       train_batch_t* out,
                                       std::string* error) const {
    if (error) error->clear();
    if (!out) return false;
    if (!stored.encoding.defined() || !stored.target.defined() ||
        !stored.future_mask.defined()) {
      if (error) *error = "stored batch has undefined tensor(s)";
      return false;
    }
    try {
      out->encoding = stored.encoding.to(
          model_.device, model_.dtype, /*non_blocking=*/true, /*copy=*/false);
      if (stored.stats.defined() && stored.stats.numel() > 0) {
        out->stats = stored.stats.to(
            model_.device, model_.dtype, /*non_blocking=*/true, /*copy=*/false);
      } else {
        out->stats = torch::Tensor();
      }
      out->target = stored.target.to(
          model_.device, model_.dtype, /*non_blocking=*/true, /*copy=*/false);
      out->future_mask = stored.future_mask.to(
          model_.device, torch::kFloat32, /*non_blocking=*/true, /*copy=*/false);
      return true;
    } catch (const std::exception& e) {
      if (error) *error = e.what();
      return false;
    }
  }

  [[nodiscard]] bool fit_target_norm_(
      const std::vector<const train_batch_t*>& batches,
      target_norm_t* out,
      std::string* error) const {
    if (error) error->clear();
    if (!out) return false;
    out->valid = false;
    out->mean.reset();
    out->std.reset();
    out->logdet_bits = 0.0;

    flat_dataset_t ds{};
    if (!flatten_batches_(batches, &ds)) {
      if (error) *error = "empty dataset for target normalization";
      return false;
    }
    if (!ds.y.defined() || !ds.w.defined() || ds.y.dim() != 2 || ds.w.dim() != 1) {
      if (error) *error = "invalid dataset tensors for target normalization";
      return false;
    }
    try {
      auto w = ds.w.clamp_min(0.0);
      const double w_sum = w.sum().item<double>();
      if (!std::isfinite(w_sum) || w_sum <= 0.0) return false;
      auto w_col = w.unsqueeze(1);
      auto mean = (ds.y * w_col).sum(/*dim=*/0) / w_sum;  // [Dy]
      auto centered = ds.y - mean;
      auto var = ((centered * centered) * w_col).sum(/*dim=*/0) / w_sum;  // [Dy]
      const double eps = std::max(1e-8, cfg_.nll_eps);
      auto std = torch::sqrt(var.clamp_min(eps));
      if (!torch::isfinite(mean).all().item<bool>() ||
          !torch::isfinite(std).all().item<bool>()) {
        if (error) *error = "non-finite normalization statistics";
        return false;
      }
      constexpr double kInvLn2 = 1.4426950408889634074;
      const double logdet_bits = std.log().sum().item<double>() * kInvLn2;
      out->mean = mean.to(torch::kCPU, torch::kFloat64);
      out->std = std.to(torch::kCPU, torch::kFloat64);
      out->logdet_bits = std::isfinite(logdet_bits) ? logdet_bits : 0.0;
      out->valid = true;
      return true;
    } catch (const std::exception& e) {
      if (error) *error = e.what();
      return false;
    }
  }

  [[nodiscard]] torch::Tensor normalize_target_(
      const torch::Tensor& target,
      const target_norm_t* norm) const {
    if (!target.defined() || !norm || !norm->valid ||
        !norm->mean.defined() || !norm->std.defined()) {
      return target;
    }
    auto mean = norm->mean.to(
        target.device(), target.dtype(), /*non_blocking=*/true, /*copy=*/false);
    auto std = norm->std.to(
        target.device(), target.dtype(), /*non_blocking=*/true, /*copy=*/false);
    const double eps = std::max(1e-8, cfg_.nll_eps);
    std = std.clamp_min(eps);
    return (target - mean.view({1, 1, 1, -1})) / std.view({1, 1, 1, -1});
  }

  [[nodiscard]] bool compute_batch_nll_(const train_batch_t& stored_batch,
                                        const target_norm_t* target_norm,
                                        double* out_nll,
                                        std::string* error) {
    if (error) error->clear();
    if (!semantic_model_) {
      if (error) *error = "probe model is not initialized";
      return false;
    }
    if (!out_nll) return false;

    train_batch_t batch{};
    if (!batch_to_runtime_(stored_batch, &batch, error)) {
      return false;
    }

    try {
      torch::NoGradGuard no_grad;
      semantic_model_->eval();
      auto out = semantic_model_->forward_from_encoding(batch.encoding);
      const cuwacunu::wikimyei::mdn::MdnNllOptions nll_opt{
          cfg_.nll_eps,
          cfg_.nll_sigma_min,
          cfg_.nll_sigma_max,
      };
      auto target = normalize_target_(batch.target, target_norm);
      auto nll_map = cuwacunu::wikimyei::mdn::mdn_nll_map(
          out, target, batch.future_mask, nll_opt);
      auto loss = weighted_masked_nll_reduce_(nll_map, batch.future_mask);
      if (!torch::isfinite(loss).item<bool>()) {
        if (error) *error = "non-finite loss";
        return false;
      }
      constexpr double kInvLn2 = 1.4426950408889634074;
      *out_nll = loss.item<double>() * kInvLn2;
      if (target_norm && target_norm->valid) {
        *out_nll += target_norm->logdet_bits;
      }
      return std::isfinite(*out_nll);
    } catch (const std::exception& e) {
      if (error) *error = e.what();
      return false;
    }
  }

  [[nodiscard]] bool train_one_batch_(const train_batch_t& batch,
                                      const target_norm_t* target_norm,
                                      std::string* error) {
    if (error) error->clear();
    if (!semantic_model_ || !optimizer_) {
      if (error) *error = "probe model is not initialized";
      return false;
    }

    train_batch_t runtime_batch{};
    if (!batch_to_runtime_(batch, &runtime_batch, error)) return false;

    semantic_model_->train();

    try {
      optimizer_->zero_grad(/*set_to_none=*/true);
      auto out = semantic_model_->forward_from_encoding(runtime_batch.encoding);
      const cuwacunu::wikimyei::mdn::MdnNllOptions nll_opt{
          cfg_.nll_eps,
          cfg_.nll_sigma_min,
          cfg_.nll_sigma_max,
      };
      auto target = normalize_target_(runtime_batch.target, target_norm);
      auto nll_map = cuwacunu::wikimyei::mdn::mdn_nll_map(
          out, target, runtime_batch.future_mask, nll_opt);
      auto loss = weighted_masked_nll_reduce_(nll_map, runtime_batch.future_mask);
      if (!torch::isfinite(loss).item<bool>()) {
        if (error) *error = "non-finite loss";
        return false;
      }

      loss.backward();
      if (cfg_.grad_clip > 0.0) {
        torch::nn::utils::clip_grad_norm_(semantic_model_->parameters(),
                                          cfg_.grad_clip);
      }
      optimizer_->step();

      ++epoch_.train_steps;
      return true;
    } catch (const std::exception& e) {
      if (error) *error = e.what();
      return false;
    }
  }

  void split_epoch_batches_(std::vector<const train_batch_t*>* train,
                            std::vector<const train_batch_t*>* val,
                            std::vector<const train_batch_t*>* test) {
    if (!train || !val || !test) return;
    train->clear();
    val->clear();
    test->clear();
    if (epoch_batches_.empty()) return;

    struct key_group_t {
      std::string key{};
      std::vector<const train_batch_t*> batches{};
      std::uint64_t hash{0};
      anchor_split_e split{anchor_split_e::Train};
    };

    std::vector<key_group_t> groups{};
    groups.reserve(epoch_batches_.size());
    std::unordered_map<std::string, std::size_t> group_index{};
    group_index.reserve(epoch_batches_.size() * 2U);

    for (std::size_t i = 0; i < epoch_batches_.size(); ++i) {
      const auto& batch = epoch_batches_[i];
      const std::string key =
          batch.anchor_key.empty() ? ("batch_" + std::to_string(i)) : batch.anchor_key;
      const auto it = group_index.find(key);
      if (it == group_index.end()) {
        key_group_t g{};
        g.key = key;
        g.hash = static_cast<std::uint64_t>(
            std::hash<std::string>{}(probe_hashimyei_ + "|" + key));
        g.batches.push_back(&batch);
        const std::size_t idx = groups.size();
        groups.push_back(std::move(g));
        group_index.emplace(key, idx);
      } else {
        groups[it->second].batches.push_back(&batch);
      }
    }

    std::sort(groups.begin(), groups.end(), [](const key_group_t& a, const key_group_t& b) {
      if (a.hash == b.hash) return a.key < b.key;
      return a.hash < b.hash;
    });

    const std::size_t n = groups.size();
    std::size_t n_train = 0;
    std::size_t n_val = 0;
    if (n == 1) {
      n_train = 1;
      n_val = 0;
    } else if (n == 2) {
      n_train = 1;
      n_val = 0;
    } else {
      n_train = static_cast<std::size_t>(
          std::llround(static_cast<double>(n) * cfg_.anchor_train_ratio));
      n_val = static_cast<std::size_t>(
          std::llround(static_cast<double>(n) * cfg_.anchor_val_ratio));
      n_train = std::max<std::size_t>(1, std::min<std::size_t>(n - 2, n_train));
      n_val = std::max<std::size_t>(1, std::min<std::size_t>(n - 1 - n_train, n_val));
      if (n_train + n_val >= n) {
        if (n_train > n_val) {
          n_train = std::max<std::size_t>(1, n - 2);
          n_val = 1;
        } else {
          n_val = std::max<std::size_t>(1, n - 2);
          n_train = 1;
        }
      }
    }
    const std::size_t n_test = (n > (n_train + n_val)) ? (n - n_train - n_val) : 0;

    for (std::size_t i = 0; i < groups.size(); ++i) {
      if (i < n_train) {
        groups[i].split = anchor_split_e::Train;
      } else if (i < (n_train + n_val)) {
        groups[i].split = anchor_split_e::Val;
      } else {
        groups[i].split = anchor_split_e::Test;
      }
      if (n_test == 0 && i >= n_train) {
        groups[i].split = anchor_split_e::Train;
      }
    }

    for (const auto& g : groups) {
      auto it_manifest = anchor_split_manifest_.find(g.key);
      if (it_manifest == anchor_split_manifest_.end() || it_manifest->second != g.split) {
        anchor_split_manifest_[g.key] = g.split;
        anchor_split_dirty_ = true;
      }
      if (g.split == anchor_split_e::Train) {
        train->insert(train->end(), g.batches.begin(), g.batches.end());
      } else if (g.split == anchor_split_e::Val) {
        val->insert(val->end(), g.batches.begin(), g.batches.end());
      } else {
        test->insert(test->end(), g.batches.begin(), g.batches.end());
      }
    }
  }

  [[nodiscard]] static std::vector<std::vector<const train_batch_t*>>
  make_prequential_blocks_(const std::vector<const train_batch_t*>& batches,
                           std::int64_t requested_blocks) {
    std::vector<std::vector<const train_batch_t*>> out;
    if (batches.empty()) return out;
    const std::size_t n = batches.size();
    const std::size_t S = static_cast<std::size_t>(
        std::max<std::int64_t>(1, std::min<std::int64_t>(requested_blocks, n)));
    out.reserve(S);
    for (std::size_t s = 0; s < S; ++s) {
      const std::size_t start = (s * n) / S;
      const std::size_t end = ((s + 1) * n) / S;
      if (end <= start) continue;
      std::vector<const train_batch_t*> block;
      block.reserve(end - start);
      for (std::size_t i = start; i < end; ++i) block.push_back(batches[i]);
      if (!block.empty()) out.push_back(std::move(block));
    }
    return out;
  }

  [[nodiscard]] bool evaluate_mdn_batches_(const std::vector<const train_batch_t*>& batches,
                                           const target_norm_t* target_norm,
                                           double* out_bits,
                                           double* out_weight,
                                           std::string* error) {
    if (error) error->clear();
    if (out_bits) *out_bits = std::numeric_limits<double>::quiet_NaN();
    if (out_weight) *out_weight = 0.0;
    if (batches.empty()) return false;

    const double dy = std::max<double>(
        1.0, static_cast<double>(model_.target_dims.size()));
    double weighted_sum = 0.0;
    double weight_sum = 0.0;
    for (const auto* batch : batches) {
      if (!batch) continue;
      double nll_bits = std::numeric_limits<double>::quiet_NaN();
      std::string batch_error;
      if (!compute_batch_nll_(*batch, target_norm, &nll_bits, &batch_error)) {
        if (error && error->empty()) *error = batch_error;
        continue;
      }
      double weight = 0.0;
      try {
        weight = batch->future_mask.sum().item<double>() * dy;
      } catch (...) {
        weight = 0.0;
      }
      if (!std::isfinite(weight) || weight <= 0.0) continue;
      weighted_sum += nll_bits * weight;
      weight_sum += weight;
    }
    if (weight_sum <= 0.0) return false;
    if (out_bits) *out_bits = weighted_sum / weight_sum;
    if (out_weight) *out_weight = weight_sum;
    return true;
  }

  void train_mdn_batches_(const std::vector<const train_batch_t*>& batches,
                          const target_norm_t* target_norm) {
    for (const auto* batch : batches) {
      if (!batch) continue;
      std::string train_error;
      if (!train_one_batch_(*batch, target_norm, &train_error)) {
        ++epoch_.optimizer_failures;
      }
    }
  }

  [[nodiscard]] bool flatten_batches_feature_(
      const std::vector<const train_batch_t*>& batches,
      feature_mode_e mode,
      flat_dataset_t* out) const {
    if (!out) return false;
    std::vector<torch::Tensor> xs{};
    std::vector<torch::Tensor> ys{};
    std::vector<torch::Tensor> ws{};
    xs.reserve(batches.size());
    ys.reserve(batches.size());
    ws.reserve(batches.size());

    for (const auto* batch : batches) {
      if (!batch || !batch->target.defined() ||
          !batch->future_mask.defined()) {
        continue;
      }
      const torch::Tensor src =
          (mode == feature_mode_e::Encoding) ? batch->encoding : batch->stats;
      if (!src.defined() || src.numel() == 0) continue;
      auto enc = src.to(torch::kCPU, torch::kFloat64);
      auto tgt = batch->target.to(torch::kCPU, torch::kFloat64);
      auto m = batch->future_mask.to(torch::kCPU, torch::kFloat64);
      if (enc.dim() != 2 || tgt.dim() != 4 || m.dim() != 3) continue;

      const auto B = enc.size(0);
      const auto C = tgt.size(1);
      const auto Hf = tgt.size(2);
      const auto Dy = tgt.size(3);
      if (B <= 0 || C <= 0 || Hf <= 0 || Dy <= 0) continue;
      if (tgt.size(0) != B || m.size(0) != B || m.size(1) != C || m.size(2) != Hf) {
        continue;
      }
      auto x = enc.unsqueeze(1).unsqueeze(1).expand({B, C, Hf, enc.size(1)})
                   .reshape({B * C * Hf, enc.size(1)});
      auto y = tgt.reshape({B * C * Hf, Dy});
      auto w = m.reshape({B * C * Hf});
      xs.push_back(std::move(x));
      ys.push_back(std::move(y));
      ws.push_back(std::move(w));
    }

    if (xs.empty()) {
      out->x.reset();
      out->y.reset();
      out->w.reset();
      return false;
    }
    out->x = torch::cat(xs, 0);
    out->y = torch::cat(ys, 0);
    out->w = torch::cat(ws, 0).clamp_min(0.0);
    return out->x.defined() && out->x.numel() > 0 &&
           out->y.defined() && out->y.numel() > 0 &&
           out->w.defined() && out->w.numel() > 0;
  }

  [[nodiscard]] bool flatten_batches_(const std::vector<const train_batch_t*>& batches,
                                      flat_dataset_t* out) const {
    return flatten_batches_feature_(batches, feature_mode_e::Encoding, out);
  }

  [[nodiscard]] bool flatten_batches_stats_(
      const std::vector<const train_batch_t*>& batches,
      flat_dataset_t* out) const {
    return flatten_batches_feature_(batches, feature_mode_e::Stats, out);
  }

  [[nodiscard]] static std::vector<const train_batch_t*> ptr_view_(
      const std::vector<train_batch_t>& batches) {
    std::vector<const train_batch_t*> out{};
    out.reserve(batches.size());
    for (const auto& b : batches) out.push_back(&b);
    return out;
  }

  [[nodiscard]] bool support_elements_(
      const std::vector<const train_batch_t*>& batches,
      double* out_support) const {
    if (out_support) *out_support = 0.0;
    flat_dataset_t ds{};
    if (!flatten_batches_(batches, &ds)) return false;
    const double w_sum = ds.w.sum().item<double>();
    if (!std::isfinite(w_sum) || w_sum <= 0.0) return false;
    const double dy = static_cast<double>(ds.y.size(1));
    const double support = w_sum * std::max(1.0, dy);
    if (!std::isfinite(support) || support <= 0.0) return false;
    if (out_support) *out_support = support;
    return true;
  }

  static void append_prefix_blocks_(
      const std::vector<std::vector<const train_batch_t*>>& blocks,
      std::size_t block_exclusive,
      std::vector<const train_batch_t*>* out_prefix) {
    if (!out_prefix) return;
    out_prefix->clear();
    for (std::size_t j = 0; j < block_exclusive; ++j) {
      out_prefix->insert(out_prefix->end(), blocks[j].begin(), blocks[j].end());
    }
  }

  [[nodiscard]] static bool fit_linear_gaussian_from_dataset_(
      const flat_dataset_t& ds,
      double ridge_lambda,
      double var_floor,
      linear_gaussian_t* out,
      std::string* error) {
    if (error) error->clear();
    if (!out) return false;
    if (!ds.x.defined() || !ds.y.defined() || !ds.w.defined()) {
      if (error) *error = "undefined dataset tensors";
      return false;
    }
    if (ds.x.dim() != 2 || ds.y.dim() != 2 || ds.w.dim() != 1) {
      if (error) *error = "invalid dataset rank";
      return false;
    }
    if (ds.x.size(0) != ds.y.size(0) || ds.x.size(0) != ds.w.size(0)) {
      if (error) *error = "dataset shape mismatch";
      return false;
    }

    const auto N = ds.x.size(0);
    const auto De = ds.x.size(1);
    const auto Dy = ds.y.size(1);
    if (N <= 0 || De <= 0 || Dy <= 0) {
      if (error) *error = "empty dataset dimensions";
      return false;
    }

    try {
      auto opts = torch::TensorOptions().dtype(torch::kFloat64).device(torch::kCPU);
      auto ones = torch::ones({N, 1}, opts);
      auto x_aug = torch::cat({ds.x, ones}, /*dim=*/1);
      auto w = ds.w.clamp_min(0.0);
      const double w_sum = w.sum().item<double>();
      if (!std::isfinite(w_sum) || w_sum <= 0.0) {
        if (error) *error = "invalid sample weights";
        return false;
      }

      auto sqrt_w = torch::sqrt(w).unsqueeze(1);
      auto xw = x_aug * sqrt_w;
      auto yw = ds.y * sqrt_w;

      const auto P = x_aug.size(1);
      auto xtx = xw.transpose(0, 1).mm(xw);
      auto xty = xw.transpose(0, 1).mm(yw);
      auto reg = torch::eye(P, opts) * std::max(0.0, ridge_lambda);
      reg.index_put_({P - 1, P - 1}, 0.0);  // keep bias unregularized

      auto lhs = xtx + reg;
      torch::Tensor beta;
      try {
        beta = torch::linalg::solve(lhs, xty, /*left=*/true);
      } catch (...) {
        beta = torch::linalg::pinv(lhs).mm(xty);
      }

      auto pred = x_aug.mm(beta);
      auto resid = ds.y - pred;
      auto w_col = w.unsqueeze(1);
      const double denom =
          std::max<double>(w_sum * static_cast<double>(Dy), 1.0);
      const double mse = ((resid * resid) * w_col).sum().item<double>() / denom;

      out->beta = beta;
      out->sigma2 = std::max(mse, var_floor);
      out->valid = out->beta.defined() && out->beta.numel() > 0 &&
                   std::isfinite(out->sigma2) && out->sigma2 > 0.0;
      return out->valid;
    } catch (const std::exception& e) {
      if (error) *error = e.what();
      return false;
    }
  }

  [[nodiscard]] static torch::Tensor predict_linear_from_dataset_(
      const linear_gaussian_t& model,
      const torch::Tensor& x) {
    TORCH_CHECK(model.valid && model.beta.defined(),
                "[transfer_matrix_eval] linear model invalid");
    TORCH_CHECK(x.defined() && x.dim() == 2, "[transfer_matrix_eval] invalid x");
    TORCH_CHECK(x.size(1) + 1 == model.beta.size(0),
                "[transfer_matrix_eval] linear model/x shape mismatch");
    auto opts = torch::TensorOptions().dtype(torch::kFloat64).device(torch::kCPU);
    auto ones = torch::ones({x.size(0), 1}, opts);
    auto x_aug = torch::cat({x, ones}, /*dim=*/1);
    return x_aug.mm(model.beta);
  }

  [[nodiscard]] static bool eval_linear_gaussian_from_dataset_(
      const linear_gaussian_t& model,
      const flat_dataset_t& ds,
      double var_floor,
      double* out_bits,
      double* out_weight,
      std::string* error) {
    if (error) error->clear();
    if (out_bits) *out_bits = std::numeric_limits<double>::quiet_NaN();
    if (out_weight) *out_weight = 0.0;
    if (!model.valid || !model.beta.defined()) {
      if (error) *error = "linear model not fitted";
      return false;
    }
    if (!ds.x.defined() || !ds.y.defined() || !ds.w.defined()) return false;
    if (ds.x.dim() != 2 || ds.y.dim() != 2 || ds.w.dim() != 1) return false;
    if (ds.x.size(0) != ds.y.size(0) || ds.x.size(0) != ds.w.size(0)) return false;
    if (ds.x.size(1) + 1 != model.beta.size(0)) {
      if (error) *error = "linear model/input shape mismatch";
      return false;
    }

    try {
      auto pred = predict_linear_from_dataset_(model, ds.x);
      auto resid = ds.y - pred;
      auto w = ds.w.clamp_min(0.0);
      const double w_sum = w.sum().item<double>();
      if (!std::isfinite(w_sum) || w_sum <= 0.0) return false;
      auto w_col = w.unsqueeze(1);
      const double denom =
          std::max<double>(w_sum * static_cast<double>(ds.y.size(1)), 1.0);
      const double mse = ((resid * resid) * w_col).sum().item<double>() / denom;

      constexpr double kTwoPi = 6.28318530717958647692;
      constexpr double kInvLn2 = 1.4426950408889634074;
      const double sigma2 = std::max(model.sigma2, var_floor);
      const double nll_bits =
          0.5 * (std::log(kTwoPi * sigma2) + (mse / sigma2)) * kInvLn2;
      if (out_bits) *out_bits = nll_bits;
      if (out_weight) *out_weight = denom;
      return std::isfinite(nll_bits);
    } catch (const std::exception& e) {
      if (error) *error = e.what();
      return false;
    }
  }

  [[nodiscard]] static bool apply_n90_from_curve_(
      const std::vector<std::pair<double, double>>& curve,
      double final_skill,
      row_result_t* out) {
    if (!out) return false;
    if (!std::isfinite(final_skill) || final_skill <= 0.0) return false;
    if (curve.empty()) return false;
    const double threshold = 0.9 * final_skill;
    for (const auto& p : curve) {
      const double n = p.first;
      const double skill = p.second;
      if (!std::isfinite(n) || !std::isfinite(skill)) continue;
      if (skill >= threshold) {
        out->n90 = n;
        out->has_n90 = true;
        return true;
      }
    }
    return false;
  }

  static void explode_batches_by_sample_(
      const std::vector<const train_batch_t*>& batches,
      std::vector<train_batch_t>* out) {
    if (!out) return;
    out->clear();
    for (const auto* batch : batches) {
      if (!batch || !batch->encoding.defined() || !batch->target.defined() ||
          !batch->future_mask.defined() || batch->encoding.dim() != 2 ||
          batch->target.dim() != 4 || batch->future_mask.dim() != 3) {
        continue;
      }
      const auto B = batch->encoding.size(0);
      if (B <= 0 || batch->target.size(0) != B || batch->future_mask.size(0) != B) {
        continue;
      }
      for (std::int64_t b = 0; b < B; ++b) {
        train_batch_t sample{};
        sample.encoding = batch->encoding.narrow(0, b, 1).clone();
        if (batch->stats.defined() && batch->stats.dim() == 2 &&
            batch->stats.size(0) == B) {
          sample.stats = batch->stats.narrow(0, b, 1).clone();
        }
        sample.target = batch->target.narrow(0, b, 1).clone();
        sample.future_mask = batch->future_mask.narrow(0, b, 1).clone();
        sample.anchor_key = batch->anchor_key + "|s=" + std::to_string(b);
        out->push_back(std::move(sample));
      }
    }
  }

  [[nodiscard]] static std::vector<train_batch_t> make_block_shuffled_control_(
      const std::vector<const train_batch_t*>& batches,
      std::int64_t block_size,
      std::uint64_t seed) {
    std::vector<train_batch_t> out{};
    out.reserve(batches.size());
    const std::int64_t bs = std::max<std::int64_t>(1, block_size);
    std::mt19937_64 rng(seed);
    for (const auto* src : batches) {
      if (!src || !src->encoding.defined() || !src->target.defined() ||
          !src->future_mask.defined() || src->target.dim() != 4 ||
          src->future_mask.dim() != 3) {
        continue;
      }
      train_batch_t cpy = *src;
      const auto B = cpy.target.size(0);
      if (B > 1 && cpy.future_mask.size(0) == B) {
        std::vector<std::int64_t> perm(static_cast<std::size_t>(B));
        std::iota(perm.begin(), perm.end(), 0);
        for (std::int64_t start = 0; start < B; start += bs) {
          const std::int64_t end = std::min<std::int64_t>(B, start + bs);
          std::shuffle(perm.begin() + start, perm.begin() + end, rng);
        }
        auto idx = torch::tensor(
            perm, torch::TensorOptions().dtype(torch::kLong).device(torch::kCPU));
        cpy.target = cpy.target.index_select(0, idx);
        cpy.future_mask = cpy.future_mask.index_select(0, idx);
      }
      cpy.anchor_key = src->anchor_key + "|ctrl";
      out.push_back(std::move(cpy));
    }
    return out;
  }

  [[nodiscard]] bool fit_null_baseline_(const std::vector<const train_batch_t*>& batches,
                                        gaussian_baseline_t* out,
                                        std::string* error) const {
    if (error) error->clear();
    if (!out) return false;
    flat_dataset_t ds{};
    if (!flatten_batches_(batches, &ds)) {
      if (error) *error = "empty dataset for null baseline";
      return false;
    }
    const double w_sum = ds.w.sum().item<double>();
    if (!std::isfinite(w_sum) || w_sum <= 0.0) return false;
    auto w_col = ds.w.unsqueeze(1);
    auto mu = (ds.y * w_col).sum(/*dim=*/0) / w_sum;
    auto resid = ds.y - mu;
    const double denom =
        std::max<double>(w_sum * static_cast<double>(ds.y.size(1)), 1.0);
    const double mse = ((resid * resid) * w_col).sum().item<double>() / denom;
    out->mu = mu;
    out->sigma2 = std::max(mse, cfg_.gaussian_var_min);
    out->valid = std::isfinite(out->sigma2) && out->sigma2 > 0.0;
    return out->valid;
  }

  [[nodiscard]] bool eval_null_baseline_bits_(
      const gaussian_baseline_t& model,
      const std::vector<const train_batch_t*>& batches,
      double* out_bits,
      double* out_weight,
      std::string* error) const {
    if (error) error->clear();
    if (out_bits) *out_bits = std::numeric_limits<double>::quiet_NaN();
    if (out_weight) *out_weight = 0.0;
    if (!model.valid || !model.mu.defined()) return false;
    flat_dataset_t ds{};
    if (!flatten_batches_(batches, &ds)) return false;
    const double w_sum = ds.w.sum().item<double>();
    if (!std::isfinite(w_sum) || w_sum <= 0.0) return false;
    auto resid = ds.y - model.mu;
    auto w_col = ds.w.unsqueeze(1);
    const double denom =
        std::max<double>(w_sum * static_cast<double>(ds.y.size(1)), 1.0);
    const double mse = ((resid * resid) * w_col).sum().item<double>() / denom;
    constexpr double kTwoPi = 6.28318530717958647692;
    constexpr double kInvLn2 = 1.4426950408889634074;
    const double sigma2 = std::max(model.sigma2, cfg_.gaussian_var_min);
    const double nll_bits =
        0.5 * (std::log(kTwoPi * sigma2) + (mse / sigma2)) * kInvLn2;
    if (out_bits) *out_bits = nll_bits;
    if (out_weight) *out_weight = denom;
    return std::isfinite(nll_bits);
  }

  [[nodiscard]] bool fit_linear_gaussian_(
      const std::vector<const train_batch_t*>& batches,
      linear_gaussian_t* out,
      std::string* error) const {
    if (error) error->clear();
    if (!out) return false;
    flat_dataset_t ds{};
    if (!flatten_batches_(batches, &ds)) {
      if (error) *error = "empty dataset for linear baseline";
      return false;
    }
    return fit_linear_gaussian_from_dataset_(
        ds, cfg_.linear_ridge_lambda, cfg_.gaussian_var_min, out, error);
  }

  [[nodiscard]] bool eval_linear_gaussian_bits_(
      const linear_gaussian_t& model,
      const std::vector<const train_batch_t*>& batches,
      double* out_bits,
      double* out_weight,
      std::string* error) const {
    if (error) error->clear();
    if (out_bits) *out_bits = std::numeric_limits<double>::quiet_NaN();
    if (out_weight) *out_weight = 0.0;
    if (!model.valid || !model.beta.defined()) {
      if (error) *error = "linear model not fitted";
      return false;
    }

    flat_dataset_t ds{};
    if (!flatten_batches_(batches, &ds)) return false;
    return eval_linear_gaussian_from_dataset_(
        model, ds, cfg_.gaussian_var_min, out_bits, out_weight, error);
  }

  [[nodiscard]] bool fit_stats_linear_gaussian_(
      const std::vector<const train_batch_t*>& batches,
      linear_gaussian_t* out,
      std::string* error) const {
    if (error) error->clear();
    if (!out) return false;
    flat_dataset_t ds{};
    if (!flatten_batches_stats_(batches, &ds)) {
      if (error) *error = "empty dataset for stats-only baseline";
      return false;
    }
    return fit_linear_gaussian_from_dataset_(
        ds, cfg_.linear_ridge_lambda, cfg_.gaussian_var_min, out, error);
  }

  [[nodiscard]] bool eval_stats_linear_gaussian_bits_(
      const linear_gaussian_t& model,
      const std::vector<const train_batch_t*>& batches,
      double* out_bits,
      double* out_weight,
      std::string* error) const {
    if (error) error->clear();
    flat_dataset_t ds{};
    if (!flatten_batches_stats_(batches, &ds)) return false;
    return eval_linear_gaussian_from_dataset_(
        model, ds, cfg_.gaussian_var_min, out_bits, out_weight, error);
  }

  [[nodiscard]] bool eval_residual_linear_bits_(
      const linear_gaussian_t& stats_model,
      const linear_gaussian_t& residual_model,
      const std::vector<const train_batch_t*>& batches,
      double* out_bits,
      double* out_weight,
      std::string* error) const {
    if (error) error->clear();
    if (out_bits) *out_bits = std::numeric_limits<double>::quiet_NaN();
    if (out_weight) *out_weight = 0.0;
    if (!stats_model.valid || !residual_model.valid) return false;

    flat_dataset_t ds_enc{};
    flat_dataset_t ds_stats{};
    if (!flatten_batches_(batches, &ds_enc) || !flatten_batches_stats_(batches, &ds_stats)) {
      return false;
    }
    if (!ds_enc.x.defined() || !ds_stats.x.defined()) return false;
    if (ds_enc.x.size(0) != ds_stats.x.size(0) || ds_enc.y.size(0) != ds_stats.y.size(0) ||
        ds_enc.w.size(0) != ds_stats.w.size(0)) {
      if (error) *error = "residual evaluation dataset alignment mismatch";
      return false;
    }

    try {
      auto y_base = predict_linear_from_dataset_(stats_model, ds_stats.x);
      auto y_res = predict_linear_from_dataset_(residual_model, ds_enc.x);
      auto y_pred = y_base + y_res;
      auto resid = ds_enc.y - y_pred;
      auto w = ds_enc.w.clamp_min(0.0);
      const double w_sum = w.sum().item<double>();
      if (!std::isfinite(w_sum) || w_sum <= 0.0) return false;
      auto w_col = w.unsqueeze(1);
      const double denom =
          std::max<double>(w_sum * static_cast<double>(ds_enc.y.size(1)), 1.0);
      const double mse = ((resid * resid) * w_col).sum().item<double>() / denom;
      constexpr double kTwoPi = 6.28318530717958647692;
      constexpr double kInvLn2 = 1.4426950408889634074;
      const double sigma2 = std::max(residual_model.sigma2, cfg_.gaussian_var_min);
      const double nll_bits =
          0.5 * (std::log(kTwoPi * sigma2) + (mse / sigma2)) * kInvLn2;
      if (out_bits) *out_bits = nll_bits;
      if (out_weight) *out_weight = denom;
      return std::isfinite(nll_bits);
    } catch (const std::exception& e) {
      if (error) *error = e.what();
      return false;
    }
  }

  void run_epoch_training_cycle_() {
    epoch_matrix_ = matrix_results_t{};
    if (epoch_batches_.empty()) {
      persist_anchor_split_manifest_();
      return;
    }

    std::vector<const train_batch_t*> train{};
    std::vector<const train_batch_t*> val{};
    std::vector<const train_batch_t*> test{};
    split_epoch_batches_(&train, &val, &test);
    if (train.empty()) train = test;
    if (train.empty()) train = val;
    if (test.empty()) test = val;
    if (test.empty()) test = train;
    std::vector<const train_batch_t*> train_val = train;
    train_val.insert(train_val.end(), val.begin(), val.end());
    if (train_val.empty()) train_val = train;

    // Prequential paths should operate at sample granularity, not batch granularity.
    std::vector<train_batch_t> train_samples_store{};
    std::vector<train_batch_t> val_samples_store{};
    std::vector<train_batch_t> test_samples_store{};
    explode_batches_by_sample_(train, &train_samples_store);
    explode_batches_by_sample_(val, &val_samples_store);
    explode_batches_by_sample_(test, &test_samples_store);

    auto train_s = ptr_view_(train_samples_store);
    auto val_s = ptr_view_(val_samples_store);
    auto test_s = ptr_view_(test_samples_store);
    if (train_s.empty()) train_s = test_s;
    if (train_s.empty()) train_s = val_s;
    if (test_s.empty()) test_s = val_s;
    if (test_s.empty()) test_s = train_s;
    std::vector<const train_batch_t*> train_val_s = train_s;
    train_val_s.insert(train_val_s.end(), val_s.begin(), val_s.end());
    if (train_val_s.empty()) train_val_s = train_s;

    target_norm_t mdn_norm{};
    const target_norm_t* mdn_norm_ptr = nullptr;
    if (fit_target_norm_(train_val_s, &mdn_norm, nullptr) ||
        fit_target_norm_(train_s, &mdn_norm, nullptr)) {
      mdn_norm_ptr = &mdn_norm;
    }

    // forecast.null
    gaussian_baseline_t null_fit{};
    if (fit_null_baseline_(train_s, &null_fit, nullptr)) {
      const auto blocks = make_prequential_blocks_(train_s, cfg_.prequential_blocks);
      double effort_sum = 0.0;
      double effort_w = 0.0;
      for (std::size_t i = 1; i < blocks.size(); ++i) {
        std::vector<const train_batch_t*> prefix{};
        append_prefix_blocks_(blocks, i, &prefix);
        gaussian_baseline_t prefix_fit{};
        if (!fit_null_baseline_(prefix, &prefix_fit, nullptr)) continue;
        double bits = std::numeric_limits<double>::quiet_NaN();
        double w = 0.0;
        if (!eval_null_baseline_bits_(prefix_fit, blocks[i], &bits, &w, nullptr)) continue;
        if (std::isfinite(bits) && w > 0.0) {
          effort_sum += bits * w;
          effort_w += w;
        }
      }
      if (effort_w > 0.0) {
        epoch_matrix_.forecast_null.effort = effort_sum / effort_w;
        epoch_matrix_.forecast_null.has_effort = true;
      }
      gaussian_baseline_t final_fit{};
      if (fit_null_baseline_(train_val_s, &final_fit, nullptr)) {
        double bits = std::numeric_limits<double>::quiet_NaN();
        double w = 0.0;
        if (eval_null_baseline_bits_(final_fit, test_s, &bits, &w, nullptr) &&
            std::isfinite(bits)) {
          epoch_matrix_.forecast_null.error = bits;
          epoch_matrix_.forecast_null.has_error = true;
          if (w > 0.0 && std::isfinite(w)) {
            epoch_matrix_.forecast_null.support = w;
            epoch_matrix_.forecast_null.has_support = true;
          }
        }
      }
    }

    // forecast.stats_only (simple side-feature baseline)
    linear_gaussian_t stats_only_fit{};
    if (fit_stats_linear_gaussian_(train_val_s, &stats_only_fit, nullptr)) {
      double bits = std::numeric_limits<double>::quiet_NaN();
      double w = 0.0;
      if (eval_stats_linear_gaussian_bits_(stats_only_fit, test_s, &bits, &w, nullptr) &&
          std::isfinite(bits)) {
        epoch_matrix_.forecast_stats_only.error = bits;
        epoch_matrix_.forecast_stats_only.has_error = true;
        if (w > 0.0 && std::isfinite(w)) {
          epoch_matrix_.forecast_stats_only.support = w;
          epoch_matrix_.forecast_stats_only.has_support = true;
        }
      }
    }

    // forecast.linear
    linear_gaussian_t linear_fit{};
    std::vector<std::pair<double, double>> linear_skill_curve{};
    if (fit_linear_gaussian_(train_s, &linear_fit, nullptr)) {
      const auto blocks = make_prequential_blocks_(train_s, cfg_.prequential_blocks);
      std::vector<double> block_supports(blocks.size(), 0.0);
      for (std::size_t i = 0; i < blocks.size(); ++i) {
        (void)support_elements_(blocks[i], &block_supports[i]);
      }
      double effort_sum = 0.0;
      double effort_w = 0.0;
      double prefix_support = 0.0;
      for (std::size_t i = 1; i < blocks.size(); ++i) {
        prefix_support += block_supports[i - 1];
        std::vector<const train_batch_t*> prefix{};
        append_prefix_blocks_(blocks, i, &prefix);
        linear_gaussian_t prefix_fit{};
        if (!fit_linear_gaussian_(prefix, &prefix_fit, nullptr)) continue;
        double row_bits = std::numeric_limits<double>::quiet_NaN();
        double w = 0.0;
        if (!eval_linear_gaussian_bits_(prefix_fit, blocks[i], &row_bits, &w, nullptr)) {
          continue;
        }
        if (std::isfinite(row_bits) && w > 0.0) {
          effort_sum += row_bits * w;
          effort_w += w;
        }
        gaussian_baseline_t prefix_null{};
        double null_bits = std::numeric_limits<double>::quiet_NaN();
        if (fit_null_baseline_(prefix, &prefix_null, nullptr) &&
            eval_null_baseline_bits_(prefix_null, blocks[i], &null_bits, nullptr, nullptr) &&
            std::isfinite(null_bits) && std::isfinite(row_bits)) {
          linear_skill_curve.emplace_back(prefix_support, null_bits - row_bits);
        }
      }
      if (effort_w > 0.0) {
        epoch_matrix_.forecast_linear.effort = effort_sum / effort_w;
        epoch_matrix_.forecast_linear.has_effort = true;
      }
      linear_gaussian_t final_fit{};
      if (fit_linear_gaussian_(train_val_s, &final_fit, nullptr)) {
        double bits = std::numeric_limits<double>::quiet_NaN();
        double w = 0.0;
        if (eval_linear_gaussian_bits_(final_fit, test_s, &bits, &w, nullptr) &&
            std::isfinite(bits)) {
          epoch_matrix_.forecast_linear.error = bits;
          epoch_matrix_.forecast_linear.has_error = true;
          if (w > 0.0 && std::isfinite(w)) {
            epoch_matrix_.forecast_linear.support = w;
            epoch_matrix_.forecast_linear.has_support = true;
          }
        }
      }
    }

    // forecast.residual.linear (stats baseline + encoding residual head)
    linear_gaussian_t stats_model{};
    linear_gaussian_t residual_model{};
    if (fit_stats_linear_gaussian_(train_val_s, &stats_model, nullptr)) {
      flat_dataset_t ds_enc_train{};
      flat_dataset_t ds_stats_train{};
      if (flatten_batches_(train_val_s, &ds_enc_train) &&
          flatten_batches_stats_(train_val_s, &ds_stats_train) &&
          ds_enc_train.x.size(0) == ds_stats_train.x.size(0) &&
          ds_enc_train.y.size(0) == ds_stats_train.y.size(0) &&
          ds_enc_train.w.size(0) == ds_stats_train.w.size(0)) {
        auto y_base_train = predict_linear_from_dataset_(stats_model, ds_stats_train.x);
        flat_dataset_t ds_res_train{};
        ds_res_train.x = ds_enc_train.x;
        ds_res_train.y = ds_enc_train.y - y_base_train;
        ds_res_train.w = ds_enc_train.w;
        if (fit_linear_gaussian_from_dataset_(
                ds_res_train,
                cfg_.linear_ridge_lambda,
                cfg_.gaussian_var_min,
                &residual_model,
                nullptr)) {
          double bits = std::numeric_limits<double>::quiet_NaN();
          double w = 0.0;
          if (eval_residual_linear_bits_(
                  stats_model, residual_model, test_s, &bits, &w, nullptr) &&
              std::isfinite(bits)) {
            epoch_matrix_.forecast_residual_linear.error = bits;
            epoch_matrix_.forecast_residual_linear.has_error = true;
            if (w > 0.0 && std::isfinite(w)) {
              epoch_matrix_.forecast_residual_linear.support = w;
              epoch_matrix_.forecast_residual_linear.has_support = true;
            }
          }
        }
      }
    }

    // forecast.mdn prequential effort + skill curve
    std::vector<std::pair<double, double>> mdn_skill_curve{};
    try {
      initialize_runtime_model_or_throw_();
      const auto blocks = make_prequential_blocks_(train_s, cfg_.prequential_blocks);
      std::vector<double> block_supports(blocks.size(), 0.0);
      for (std::size_t i = 0; i < blocks.size(); ++i) {
        (void)support_elements_(blocks[i], &block_supports[i]);
      }

      double effort_sum = 0.0;
      double effort_w = 0.0;
      double prefix_support = 0.0;
      std::vector<const train_batch_t*> prefix{};
      for (std::size_t i = 0; i < blocks.size(); ++i) {
        if (i > 0) {
          double row_bits = std::numeric_limits<double>::quiet_NaN();
          double w = 0.0;
          if (evaluate_mdn_batches_(blocks[i], mdn_norm_ptr, &row_bits, &w, nullptr) &&
              std::isfinite(row_bits) && w > 0.0) {
            effort_sum += row_bits * w;
            effort_w += w;
          }
          gaussian_baseline_t prefix_null{};
          double null_bits = std::numeric_limits<double>::quiet_NaN();
          if (fit_null_baseline_(prefix, &prefix_null, nullptr) &&
              eval_null_baseline_bits_(prefix_null, blocks[i], &null_bits, nullptr, nullptr) &&
              std::isfinite(null_bits) && std::isfinite(row_bits)) {
            mdn_skill_curve.emplace_back(prefix_support, null_bits - row_bits);
          }
        }
        train_mdn_batches_(blocks[i], mdn_norm_ptr);
        prefix.insert(prefix.end(), blocks[i].begin(), blocks[i].end());
        prefix_support += block_supports[i];
      }
      if (effort_w > 0.0) {
        epoch_matrix_.forecast_mdn.effort = effort_sum / effort_w;
        epoch_matrix_.forecast_mdn.has_effort = true;
      }
    } catch (const std::exception&) {
      ++epoch_.optimizer_failures;
    }

    // forecast.mdn held-out error + health
    try {
      initialize_runtime_model_or_throw_();
      double cold_bits = std::numeric_limits<double>::quiet_NaN();
      if (evaluate_mdn_batches_(test_s, mdn_norm_ptr, &cold_bits, nullptr, nullptr) &&
          std::isfinite(cold_bits)) {
        epoch_matrix_.cold_start_loss = cold_bits;
        epoch_matrix_.has_cold_start_loss = true;
      }
      train_mdn_batches_(train_val_s, mdn_norm_ptr);
      double train_fit_bits = std::numeric_limits<double>::quiet_NaN();
      if (evaluate_mdn_batches_(train_val_s, mdn_norm_ptr, &train_fit_bits, nullptr, nullptr) &&
          std::isfinite(train_fit_bits)) {
        epoch_matrix_.train_fit_loss = train_fit_bits;
        epoch_matrix_.has_train_fit_loss = true;
      }
      double err_bits = std::numeric_limits<double>::quiet_NaN();
      double err_w = 0.0;
      if (evaluate_mdn_batches_(test_s, mdn_norm_ptr, &err_bits, &err_w, nullptr) &&
          std::isfinite(err_bits)) {
        epoch_matrix_.forecast_mdn.error = err_bits;
        epoch_matrix_.forecast_mdn.has_error = true;
        if (err_w > 0.0 && std::isfinite(err_w)) {
          epoch_matrix_.forecast_mdn.support = err_w;
          epoch_matrix_.forecast_mdn.has_support = true;
        }
      }
    } catch (const std::exception&) {
      ++epoch_.optimizer_failures;
    }

    if (epoch_matrix_.has_train_fit_loss && epoch_matrix_.forecast_mdn.has_error) {
      epoch_matrix_.generalization_gap =
          epoch_matrix_.train_fit_loss - epoch_matrix_.forecast_mdn.error;
      epoch_matrix_.has_generalization_gap =
          std::isfinite(epoch_matrix_.generalization_gap);
    }

    // Skills relative to forecast.null.
    if (epoch_matrix_.forecast_null.has_effort && epoch_matrix_.forecast_mdn.has_effort) {
      epoch_matrix_.forecast_null.effort_skill = 0.0;
      epoch_matrix_.forecast_null.has_effort_skill = true;
      epoch_matrix_.forecast_mdn.effort_skill =
          epoch_matrix_.forecast_null.effort - epoch_matrix_.forecast_mdn.effort;
      epoch_matrix_.forecast_mdn.has_effort_skill = true;
    }
    if (epoch_matrix_.forecast_null.has_effort && epoch_matrix_.forecast_linear.has_effort) {
      epoch_matrix_.forecast_null.effort_skill = 0.0;
      epoch_matrix_.forecast_null.has_effort_skill = true;
      epoch_matrix_.forecast_linear.effort_skill =
          epoch_matrix_.forecast_null.effort - epoch_matrix_.forecast_linear.effort;
      epoch_matrix_.forecast_linear.has_effort_skill = true;
    }
    if (epoch_matrix_.forecast_null.has_error && epoch_matrix_.forecast_stats_only.has_error) {
      epoch_matrix_.forecast_null.error_skill = 0.0;
      epoch_matrix_.forecast_null.has_error_skill = true;
      epoch_matrix_.forecast_stats_only.error_skill =
          epoch_matrix_.forecast_null.error - epoch_matrix_.forecast_stats_only.error;
      epoch_matrix_.forecast_stats_only.has_error_skill = true;
    }
    if (epoch_matrix_.forecast_null.has_error && epoch_matrix_.forecast_mdn.has_error) {
      epoch_matrix_.forecast_null.error_skill = 0.0;
      epoch_matrix_.forecast_null.has_error_skill = true;
      epoch_matrix_.forecast_mdn.error_skill =
          epoch_matrix_.forecast_null.error - epoch_matrix_.forecast_mdn.error;
      epoch_matrix_.forecast_mdn.has_error_skill = true;
    }
    if (epoch_matrix_.forecast_null.has_error && epoch_matrix_.forecast_linear.has_error) {
      epoch_matrix_.forecast_null.error_skill = 0.0;
      epoch_matrix_.forecast_null.has_error_skill = true;
      epoch_matrix_.forecast_linear.error_skill =
          epoch_matrix_.forecast_null.error - epoch_matrix_.forecast_linear.error;
      epoch_matrix_.forecast_linear.has_error_skill = true;
    }
    if (epoch_matrix_.forecast_null.has_error &&
        epoch_matrix_.forecast_residual_linear.has_error) {
      epoch_matrix_.forecast_null.error_skill = 0.0;
      epoch_matrix_.forecast_null.has_error_skill = true;
      epoch_matrix_.forecast_residual_linear.error_skill =
          epoch_matrix_.forecast_null.error - epoch_matrix_.forecast_residual_linear.error;
      epoch_matrix_.forecast_residual_linear.has_error_skill = true;
    }

    // n90 from prequential skill curves.
    if (epoch_matrix_.forecast_linear.has_error_skill &&
        epoch_matrix_.forecast_linear.error_skill > 0.0) {
      (void)apply_n90_from_curve_(linear_skill_curve,
                                  epoch_matrix_.forecast_linear.error_skill,
                                  &epoch_matrix_.forecast_linear);
    }
    if (epoch_matrix_.forecast_mdn.has_error_skill &&
        epoch_matrix_.forecast_mdn.error_skill > 0.0) {
      (void)apply_n90_from_curve_(mdn_skill_curve,
                                  epoch_matrix_.forecast_mdn.error_skill,
                                  &epoch_matrix_.forecast_mdn);
    }

    // Selectivity controls for linear/mdn using block-shuffled targets.
    if (!train_val.empty() && !test.empty()) {
      const std::uint64_t base_seed =
          static_cast<std::uint64_t>(std::max<std::int64_t>(0, cfg_.control_shuffle_seed));
      auto ctrl_train_val_store = make_block_shuffled_control_(
          train_val,
          cfg_.control_shuffle_block,
          base_seed + 17ULL);
      auto ctrl_test_store = make_block_shuffled_control_(
          test,
          cfg_.control_shuffle_block,
          base_seed + 29ULL);
      auto ctrl_train_val = ptr_view_(ctrl_train_val_store);
      auto ctrl_test = ptr_view_(ctrl_test_store);

      // linear selectivity
      linear_gaussian_t ctrl_linear{};
      gaussian_baseline_t ctrl_null{};
      double ctrl_null_bits = std::numeric_limits<double>::quiet_NaN();
      double ctrl_linear_bits = std::numeric_limits<double>::quiet_NaN();
      if (fit_null_baseline_(ctrl_train_val, &ctrl_null, nullptr) &&
          eval_null_baseline_bits_(ctrl_null, ctrl_test, &ctrl_null_bits, nullptr, nullptr) &&
          fit_linear_gaussian_(ctrl_train_val, &ctrl_linear, nullptr) &&
          eval_linear_gaussian_bits_(ctrl_linear, ctrl_test, &ctrl_linear_bits, nullptr, nullptr) &&
          std::isfinite(ctrl_null_bits) && std::isfinite(ctrl_linear_bits) &&
          epoch_matrix_.forecast_linear.has_error_skill &&
          epoch_matrix_.forecast_linear.error_skill > 0.0) {
        const double ctrl_skill = ctrl_null_bits - ctrl_linear_bits;
        epoch_matrix_.forecast_linear.selectivity =
            epoch_matrix_.forecast_linear.error_skill - ctrl_skill;
        epoch_matrix_.forecast_linear.has_selectivity =
            std::isfinite(epoch_matrix_.forecast_linear.selectivity);
      }

      // mdn selectivity
      double ctrl_mdn_bits = std::numeric_limits<double>::quiet_NaN();
      double ctrl_null_bits_mdn = std::numeric_limits<double>::quiet_NaN();
      if (fit_null_baseline_(ctrl_train_val, &ctrl_null, nullptr) &&
          eval_null_baseline_bits_(
              ctrl_null, ctrl_test, &ctrl_null_bits_mdn, nullptr, nullptr) &&
          std::isfinite(ctrl_null_bits_mdn) &&
          epoch_matrix_.forecast_mdn.has_error_skill &&
          epoch_matrix_.forecast_mdn.error_skill > 0.0) {
        const auto saved_steps = epoch_.train_steps;
        const auto saved_failures = epoch_.optimizer_failures;
        try {
          initialize_runtime_model_or_throw_();
          train_mdn_batches_(ctrl_train_val, mdn_norm_ptr);
          if (evaluate_mdn_batches_(ctrl_test, mdn_norm_ptr, &ctrl_mdn_bits, nullptr, nullptr) &&
              std::isfinite(ctrl_mdn_bits)) {
            const double ctrl_skill = ctrl_null_bits_mdn - ctrl_mdn_bits;
            epoch_matrix_.forecast_mdn.selectivity =
                epoch_matrix_.forecast_mdn.error_skill - ctrl_skill;
            epoch_matrix_.forecast_mdn.has_selectivity =
                std::isfinite(epoch_matrix_.forecast_mdn.selectivity);
          }
        } catch (const std::exception&) {
          // no-op
        }
        epoch_.train_steps = saved_steps;
        epoch_.optimizer_failures = saved_failures;
      }
    }

    persist_anchor_split_manifest_();
  }

  void emit_periodic_summary_(const Wave& wave, Emitter& out, bool force) const {
    const bool cadence_hit =
        (cfg_.summary_every_steps > 0) &&
        (epoch_.seen > 0) &&
        ((epoch_.seen % static_cast<std::uint64_t>(cfg_.summary_every_steps)) == 0);
    if (!force && !cadence_hit) return;

    const std::uint64_t invalid =
        (epoch_.seen >= epoch_.accepted) ? (epoch_.seen - epoch_.accepted) : 0;

    std::ostringstream oss;
    oss << "transfer_matrix_eval.summary"
        << " seen=" << epoch_.seen
        << " accepted=" << epoch_.accepted
        << " buffered_batches=" << epoch_batches_.size()
        << " invalid=" << invalid
        << " train_steps=" << epoch_.train_steps
        << " null_cargo=" << epoch_.null_cargo
        << " invalid_cargo=" << epoch_.invalid_cargo
        << " missing_encoding=" << epoch_.missing_encoding
        << " non_finite_encoding=" << epoch_.non_finite_encoding
        << " temporal_overlap=" << epoch_.temporal_overlap
        << " shape_mismatch=" << epoch_.shape_mismatch
        << " optimizer_failures=" << epoch_.optimizer_failures;
    emit_meta_(wave, out, oss.str());
  }

  [[nodiscard]] epoch_report_metrics_t collect_epoch_report_metrics_() const {
    epoch_report_metrics_t m{};
    m.invalid =
        (epoch_.seen >= epoch_.accepted) ? (epoch_.seen - epoch_.accepted) : 0;
    m.buffered_batches = static_cast<std::uint64_t>(epoch_batches_.size());
    m.matrix = epoch_matrix_;
    return m;
  }

  [[nodiscard]] static std::string trim_ascii_copy_(std::string s) {
    std::size_t b = 0;
    while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    std::size_t e = s.size();
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
  }

  [[nodiscard]] static bool parse_bool_token_(std::string_view token,
                                              bool* out) {
    if (!out) return false;
    const std::string t = trim_ascii_copy_(std::string(token));
    if (t == "1" || t == "true" || t == "TRUE") {
      *out = true;
      return true;
    }
    if (t == "0" || t == "false" || t == "FALSE") {
      *out = false;
      return true;
    }
    return false;
  }

  [[nodiscard]] static bool parse_double_token_(std::string_view token,
                                                double* out) {
    if (!out) return false;
    try {
      *out = std::stod(trim_ascii_copy_(std::string(token)));
      return true;
    } catch (...) {
      return false;
    }
  }

  [[nodiscard]] static bool parse_uint64_token_(std::string_view token,
                                                std::uint64_t* out) {
    if (!out) return false;
    try {
      *out = static_cast<std::uint64_t>(
          std::stoull(trim_ascii_copy_(std::string(token))));
      return true;
    } catch (...) {
      return false;
    }
  }

  [[nodiscard]] static std::vector<std::string> split_pipe_(std::string_view s) {
    std::vector<std::string> out;
    std::size_t pos = 0;
    while (pos <= s.size()) {
      const std::size_t sep = s.find('|', pos);
      if (sep == std::string_view::npos) {
        out.emplace_back(s.substr(pos));
        break;
      }
      out.emplace_back(s.substr(pos, sep - pos));
      pos = sep + 1;
    }
    return out;
  }

  [[nodiscard]] static const char* split_token_(anchor_split_e split) {
    switch (split) {
      case anchor_split_e::Train:
        return "train";
      case anchor_split_e::Val:
        return "val";
      case anchor_split_e::Test:
        return "test";
      default:
        return "train";
    }
  }

  [[nodiscard]] static bool parse_split_token_(std::string_view token,
                                               anchor_split_e* out) {
    if (!out) return false;
    const std::string t = trim_ascii_copy_(std::string(token));
    if (t == "train") {
      *out = anchor_split_e::Train;
      return true;
    }
    if (t == "val") {
      *out = anchor_split_e::Val;
      return true;
    }
    if (t == "test") {
      *out = anchor_split_e::Test;
      return true;
    }
    return false;
  }

  void initialize_probe_hashimyei_state_() {
    namespace fs = std::filesystem;
    if (probe_hashimyei_.empty()) {
      throw std::runtime_error(
          "transfer_matrix_eval requires explicit probe hashimyei suffix "
          "(expected tsi.probe.representation.transfer_matrix_evaluation.0x<hex>)");
    }
    if (!cuwacunu::hashimyei::is_hex_hash_name(probe_hashimyei_)) {
      throw std::runtime_error("transfer_matrix_eval invalid probe hashimyei: " +
                               probe_hashimyei_);
    }
    probe_canonical_base_ = std::string(type_name()) + "." + probe_hashimyei_;
    probe_store_root_ = cuwacunu::hashimyei::store_root() / "tsi.probe" /
                        "representation" / "transfer_matrix_evaluation";
    probe_artifact_directory_ = probe_store_root_ / probe_hashimyei_;
    probe_metric_history_path_ =
        probe_artifact_directory_ / "metrics.history.v1.txt";
    probe_anchor_split_path_ =
        probe_artifact_directory_ / "anchor_split.v1.txt";

    std::error_code ec;
    fs::create_directories(probe_artifact_directory_, ec);
    if (ec) {
      log_warn(
          "[transfer_matrix_eval] cannot create probe hashimyei directory: %s\n",
          probe_artifact_directory_.string().c_str());
      probe_metric_history_path_.clear();
      return;
    }
    if (cfg_.reset_hashimyei_on_start) {
      metric_history_.clear();
      persisted_epoch_count_ = 0;
      anchor_split_manifest_.clear();
      anchor_split_dirty_ = false;

      std::error_code rm_ec;
      if (!probe_metric_history_path_.empty()) {
        fs::remove(probe_metric_history_path_, rm_ec);
        if (rm_ec) {
          log_warn("[transfer_matrix_eval] unable to reset metric history file: %s\n",
                   probe_metric_history_path_.string().c_str());
        }
      }
      rm_ec.clear();
      if (!probe_anchor_split_path_.empty()) {
        fs::remove(probe_anchor_split_path_, rm_ec);
        if (rm_ec) {
          log_warn("[transfer_matrix_eval] unable to reset anchor split file: %s\n",
                   probe_anchor_split_path_.string().c_str());
        }
      }
      return;
    }
    load_probe_metric_history_();
    load_anchor_split_manifest_();
  }

  void load_anchor_split_manifest_() {
    anchor_split_manifest_.clear();
    anchor_split_dirty_ = false;
    if (probe_anchor_split_path_.empty()) return;
    std::ifstream in(probe_anchor_split_path_, std::ios::binary);
    if (!in) return;

    std::string line;
    while (std::getline(in, line)) {
      line = trim_ascii_copy_(std::move(line));
      if (line.empty() || line[0] == '#') continue;
      const std::size_t tab = line.find('\t');
      if (tab == std::string::npos || tab == 0 || tab + 1 >= line.size()) continue;
      const std::string key = line.substr(0, tab);
      const std::string split = line.substr(tab + 1);
      anchor_split_e parsed = anchor_split_e::Train;
      if (!parse_split_token_(split, &parsed)) continue;
      anchor_split_manifest_[key] = parsed;
    }
  }

  void persist_anchor_split_manifest_() {
    if (!anchor_split_dirty_ || probe_anchor_split_path_.empty()) return;
    std::ofstream out(probe_anchor_split_path_,
                      std::ios::binary | std::ios::trunc);
    if (!out) {
      log_warn("[transfer_matrix_eval] unable to persist anchor split file: %s\n",
               probe_anchor_split_path_.string().c_str());
      return;
    }

    out << "schema=tsi.probe.representation.transfer_matrix_evaluation.anchor_split.v1\n";
    out << "hashimyei=" << probe_hashimyei_ << "\n";
    out << "canonical_base=" << probe_canonical_base_ << "\n";

    std::vector<std::string> keys;
    keys.reserve(anchor_split_manifest_.size());
    for (const auto& kv : anchor_split_manifest_) keys.push_back(kv.first);
    std::sort(keys.begin(), keys.end());
    for (const auto& key : keys) {
      const auto it = anchor_split_manifest_.find(key);
      if (it == anchor_split_manifest_.end()) continue;
      out << key << "\t" << split_token_(it->second) << "\n";
    }
    anchor_split_dirty_ = false;
  }

  [[nodiscard]] anchor_split_e split_for_anchor_key_(const std::string& key) {
    const auto it = anchor_split_manifest_.find(key);
    if (it != anchor_split_manifest_.end()) return it->second;

    const auto h = std::hash<std::string>{}(probe_hashimyei_ + "|" + key);
    const double u = static_cast<double>(h % 1000000ULL) / 1000000.0;
    const double t = std::clamp(cfg_.anchor_train_ratio, 0.0, 1.0);
    const double v = std::clamp(cfg_.anchor_val_ratio, 0.0, 1.0);

    anchor_split_e assigned = anchor_split_e::Test;
    if (u < t) {
      assigned = anchor_split_e::Train;
    } else if (u < (t + v)) {
      assigned = anchor_split_e::Val;
    } else {
      assigned = anchor_split_e::Test;
    }
    anchor_split_manifest_[key] = assigned;
    anchor_split_dirty_ = true;
    return assigned;
  }

  void load_probe_metric_history_() {
    if (probe_metric_history_path_.empty()) return;
    std::ifstream in(probe_metric_history_path_, std::ios::binary);
    if (!in) return;

    std::string line;
    while (std::getline(in, line)) {
      line = trim_ascii_copy_(std::move(line));
      if (line.empty() || line[0] == '#') continue;
      const std::size_t eq = line.find('=');
      if (eq == std::string::npos || eq == 0 || eq + 1 >= line.size()) continue;

      const std::string key = trim_ascii_copy_(line.substr(0, eq));
      const std::string value = trim_ascii_copy_(line.substr(eq + 1));
      if (key == "epoch_count") {
        std::uint64_t parsed = 0;
        if (parse_uint64_token_(value, &parsed)) {
          persisted_epoch_count_ = parsed;
        }
        continue;
      }

      constexpr std::string_view kMetricPrefix = "metric.";
      if (key.size() <= kMetricPrefix.size() ||
          key.compare(0, kMetricPrefix.size(), kMetricPrefix) != 0) {
        continue;
      }
      const std::string metric_key = key.substr(kMetricPrefix.size());
      const auto fields = split_pipe_(value);
      if (fields.size() != 5 && fields.size() != 9) continue;

      bool higher_is_better = false;
      bool has_value = false;
      double best = 0.0;
      double worst = 0.0;
      double min_seen = 0.0;
      double max_seen = 0.0;
      double mean = 0.0;
      double m2 = 0.0;
      std::uint64_t updates = 0;
      if (!parse_bool_token_(fields[0], &higher_is_better)) continue;
      if (!parse_bool_token_(fields[1], &has_value)) continue;
      if (!parse_double_token_(fields[2], &best)) continue;
      if (!parse_double_token_(fields[3], &worst)) continue;
      if (!parse_uint64_token_(fields[4], &updates)) continue;
      if (fields.size() == 9) {
        if (!parse_double_token_(fields[5], &min_seen)) continue;
        if (!parse_double_token_(fields[6], &max_seen)) continue;
        if (!parse_double_token_(fields[7], &mean)) continue;
        if (!parse_double_token_(fields[8], &m2)) continue;
      } else {
        min_seen = std::min(best, worst);
        max_seen = std::max(best, worst);
        mean = best;
        m2 = 0.0;
      }

      history_metric_t m{};
      m.higher_is_better = higher_is_better;
      m.has_value = has_value && std::isfinite(best) && std::isfinite(worst);
      m.best = best;
      m.worst = worst;
      m.min_seen = min_seen;
      m.max_seen = max_seen;
      m.mean = mean;
      m.m2 = std::max(0.0, m2);
      m.updates = updates;
      if (!std::isfinite(m.min_seen) || !std::isfinite(m.max_seen) ||
          !std::isfinite(m.mean) || !std::isfinite(m.m2) ||
          m.max_seen < m.min_seen) {
        m.has_value = false;
      }
      metric_history_[metric_key] = m;
    }
  }

  void persist_probe_metric_history_() const {
    if (probe_metric_history_path_.empty()) return;
    std::ofstream out(probe_metric_history_path_,
                      std::ios::binary | std::ios::trunc);
    if (!out) {
      log_warn("[transfer_matrix_eval] unable to persist metric history: %s\n",
               probe_metric_history_path_.string().c_str());
      return;
    }

    out << "schema=tsi.probe.representation.transfer_matrix_evaluation.history.v2\n";
    out << "hashimyei=" << probe_hashimyei_ << "\n";
    out << "canonical_base=" << probe_canonical_base_ << "\n";
    out << "contract_hash=" << contract_hash_ << "\n";
    out << "instance_name=" << std::string(instance_name()) << "\n";
    out << "epoch_count=" << persisted_epoch_count_ << "\n";

    std::vector<std::string> keys;
    keys.reserve(metric_history_.size());
    for (const auto& kv : metric_history_) keys.push_back(kv.first);
    std::sort(keys.begin(), keys.end());

    for (const auto& key : keys) {
      const auto it = metric_history_.find(key);
      if (it == metric_history_.end()) continue;
      const auto& m = it->second;
      out << "metric." << key << "=" << (m.higher_is_better ? "1" : "0")
          << "|" << (m.has_value ? "1" : "0") << "|" << std::setprecision(17)
          << m.best << "|" << m.worst << "|" << m.updates << "|" << m.min_seen
          << "|" << m.max_seen << "|" << m.mean << "|" << m.m2 << "\n";
    }
  }

  void update_history_metric_(std::string_view key,
                              double value,
                              bool higher_is_better) {
    if (!std::isfinite(value)) return;
    auto& m = metric_history_[std::string(key)];
    if (m.updates == 0 && !m.has_value) m.higher_is_better = higher_is_better;
    if (m.has_value && m.higher_is_better != higher_is_better) {
      // Allow future policy flips without requiring manual metric-history edits.
      m.higher_is_better = higher_is_better;
      m.best = higher_is_better ? m.max_seen : m.min_seen;
      m.worst = higher_is_better ? m.min_seen : m.max_seen;
    }
    if (!m.has_value) {
      m.has_value = true;
      m.best = value;
      m.worst = value;
      m.min_seen = value;
      m.max_seen = value;
      m.mean = value;
      m.m2 = 0.0;
      m.updates = 1;
      return;
    }

    const std::uint64_t prev_n = m.updates;
    const std::uint64_t next_n = prev_n + 1;
    const double delta = value - m.mean;
    m.mean += delta / static_cast<double>(next_n);
    const double delta2 = value - m.mean;
    m.m2 += delta * delta2;
    if (m.m2 < 0.0) m.m2 = 0.0;

    m.min_seen = std::min(m.min_seen, value);
    m.max_seen = std::max(m.max_seen, value);
    if (m.higher_is_better) {
      m.best = std::max(m.best, value);
      m.worst = std::min(m.worst, value);
    } else {
      m.best = std::min(m.best, value);
      m.worst = std::max(m.worst, value);
    }
    m.updates = next_n;
  }

  void update_persisted_epoch_history_() {
    const auto metrics = collect_epoch_report_metrics_();
    update_history_metric_("seen", static_cast<double>(epoch_.seen), true);
    update_history_metric_("accepted", static_cast<double>(epoch_.accepted), true);
    update_history_metric_("invalid", static_cast<double>(metrics.invalid), false);
    update_history_metric_("buffered_batches",
                           static_cast<double>(metrics.buffered_batches),
                           true);
    update_history_metric_("train_steps",
                           static_cast<double>(epoch_.train_steps),
                           true);

    update_history_metric_("null_cargo", static_cast<double>(epoch_.null_cargo),
                           false);
    update_history_metric_(
        "invalid_cargo", static_cast<double>(epoch_.invalid_cargo), false);
    update_history_metric_(
        "missing_encoding", static_cast<double>(epoch_.missing_encoding), false);
    update_history_metric_("non_finite_encoding",
                           static_cast<double>(epoch_.non_finite_encoding),
                           false);
    update_history_metric_(
        "temporal_overlap", static_cast<double>(epoch_.temporal_overlap), false);
    update_history_metric_("missing_future",
                           static_cast<double>(epoch_.missing_future), false);
    update_history_metric_("future_values_present",
                           static_cast<double>(epoch_.future_values_present),
                           false);
    update_history_metric_("shape_mismatch",
                           static_cast<double>(epoch_.shape_mismatch), false);
    update_history_metric_("optimizer_failures",
                           static_cast<double>(epoch_.optimizer_failures),
                           false);

    const auto& mx = metrics.matrix;
    const auto update_row_history =
        [&](std::string_view prefix, const row_result_t& row) {
          const std::string p(prefix);
          if (row.has_support) {
            update_history_metric_(p + "_support", row.support, true);
          }
          if (row.has_effort) {
            update_history_metric_(p + "_effort", row.effort, false);
          }
          if (row.has_error) {
            update_history_metric_(p + "_error", row.error, false);
          }
          if (row.has_effort_skill) {
            update_history_metric_(p + "_effort_skill", row.effort_skill, true);
          }
          if (row.has_error_skill) {
            update_history_metric_(p + "_error_skill", row.error_skill, true);
          }
          if (row.has_n90) {
            update_history_metric_(p + "_n90", row.n90, false);
          }
          if (row.has_selectivity) {
            update_history_metric_(p + "_selectivity", row.selectivity, true);
          }
        };
    update_row_history("forecast_null", mx.forecast_null);
    update_row_history("forecast_stats_only", mx.forecast_stats_only);
    update_row_history("forecast_linear", mx.forecast_linear);
    update_row_history("forecast_residual_linear", mx.forecast_residual_linear);
    update_row_history("forecast_mdn", mx.forecast_mdn);
    if (mx.has_cold_start_loss) {
      update_history_metric_("cold_start_loss", mx.cold_start_loss, false);
    }
    if (mx.has_train_fit_loss) {
      update_history_metric_("train_fit_loss", mx.train_fit_loss, false);
    }
    if (mx.has_generalization_gap) {
      update_history_metric_("generalization_gap", mx.generalization_gap, false);
    }

    ++persisted_epoch_count_;
    persist_probe_metric_history_();
  }

  [[nodiscard]] static std::string format_metric_number_(double value,
                                                         bool integer_style) {
    if (!std::isfinite(value)) return "nan";
    std::ostringstream oss;
    if (integer_style) {
      const auto as_u64 =
          static_cast<std::uint64_t>(std::llround(std::max(0.0, value)));
      oss << as_u64;
      return oss.str();
    }
    const double abs_v = std::fabs(value);
    if (abs_v >= 1.0e6 || (abs_v > 0.0 && abs_v < 1.0e-3)) {
      oss << std::scientific << std::setprecision(3) << value;
    } else {
      oss << std::fixed << std::setprecision(6) << value;
    }
    return oss.str();
  }

  [[nodiscard]] double metric_score_(const report_cell_t& cell) const {
    if (!cell.valid || !std::isfinite(cell.value)) return 0.0;
    const auto it = metric_history_.find(cell.key);
    if (it == metric_history_.end()) return 0.5;
    const auto& h = it->second;
    if (!h.has_value || !std::isfinite(h.best) || !std::isfinite(h.worst) ||
        !std::isfinite(h.min_seen) || !std::isfinite(h.max_seen) ||
        !std::isfinite(h.mean) || !std::isfinite(h.m2)) {
      return 0.5;
    }

    const bool higher_is_better = cell.higher_is_better;
    constexpr double kEps = 1.0e-9;
    const double min_seen = h.min_seen;
    const double max_seen = h.max_seen;
    const double range = max_seen - min_seen;

    double range_score = 0.5;
    if (std::isfinite(range) && range > kEps) {
      if (higher_is_better) {
        range_score = std::clamp((cell.value - min_seen) / range, 0.0, 1.0);
      } else {
        range_score = std::clamp((max_seen - cell.value) / range, 0.0, 1.0);
      }
    }

    double z_score = range_score;
    if (h.updates >= 2) {
      const double var = h.m2 / static_cast<double>(h.updates - 1);
      if (std::isfinite(var) && var > kEps) {
        const double stddev = std::sqrt(var);
        const double signed_delta = higher_is_better
                                        ? (cell.value - h.mean)
                                        : (h.mean - cell.value);
        const double z = signed_delta / stddev;
        z_score = std::clamp(0.5 + 0.20 * z, 0.0, 1.0);
      }
    }

    double blend_weight = 0.25;
    if (h.updates >= 16) {
      blend_weight = 0.65;
    } else if (h.updates >= 8) {
      blend_weight = 0.50;
    } else if (h.updates >= 4) {
      blend_weight = 0.35;
    }
    const double score =
        (1.0 - blend_weight) * range_score + blend_weight * z_score;
    return std::clamp(score, 0.0, 1.0);
  }

  [[nodiscard]] static std::string gradient_color_(double score) {
    const double s = std::clamp(score, 0.0, 1.0);
    const int r = static_cast<int>(std::lround((1.0 - s) * 220.0 + s * 40.0));
    const int g = static_cast<int>(std::lround((1.0 - s) * 60.0 + s * 230.0));
    const int b = static_cast<int>(std::lround((1.0 - s) * 48.0 + s * 70.0));
    std::ostringstream oss;
    oss << "\x1b[38;2;" << r << ";" << g << ";" << b << "m";
    return oss.str();
  }

  [[nodiscard]] std::string format_metric_cell_(const report_cell_t& cell,
                                                std::size_t col_width) const {
    const std::string value =
        (!cell.valid || !std::isfinite(cell.value))
            ? std::string("n/a")
            : format_metric_number_(cell.value, cell.integer_style);
    std::ostringstream padded;
    padded << std::left << std::setw(static_cast<int>(col_width)) << value;
    if (!cell.valid || !std::isfinite(cell.value)) {
      return std::string(ANSI_COLOR_Dim_Gray) + padded.str() + ANSI_COLOR_RESET;
    }
    return gradient_color_(metric_score_(cell)) + padded.str() + ANSI_COLOR_RESET;
  }

  void append_matrix_row_(std::ostringstream* oss,
                          std::string_view group,
                          const std::vector<report_cell_t>& cells) const {
    if (!oss) return;
    constexpr std::size_t kGroupWidth = 18;
    constexpr std::size_t kColWidth = 18;
    *oss << "  " << ANSI_COLOR_Dim_White << std::left << std::setw(static_cast<int>(kGroupWidth))
         << group
         << ANSI_COLOR_RESET << "\n";
    *oss << "    ";
    for (std::size_t i = 0; i < cells.size(); ++i) {
      *oss << ANSI_COLOR_Dim_Cyan << std::left << std::setw(static_cast<int>(kColWidth))
           << cells[i].label << ANSI_COLOR_RESET;
    }
    *oss << "\n    ";
    for (std::size_t i = 0; i < cells.size(); ++i) {
      *oss << format_metric_cell_(cells[i], kColWidth);
    }
  }

  [[nodiscard]] std::string build_epoch_report_string_(bool beautify) const {
    const auto metrics = collect_epoch_report_metrics_();
    const auto& mx = metrics.matrix;

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6);

    if (!beautify) {
      oss << "transfer_matrix_eval.epoch"
          << " hashimyei=" << probe_hashimyei_
          << " seen=" << epoch_.seen
          << " accepted=" << epoch_.accepted
          << " buffered_batches=" << metrics.buffered_batches
          << " invalid=" << metrics.invalid
          << " train_steps=" << epoch_.train_steps
          << " forecast_null_support=" << mx.forecast_null.support
          << " forecast_null_effort=" << mx.forecast_null.effort
          << " forecast_null_error=" << mx.forecast_null.error
          << " forecast_stats_only_support=" << mx.forecast_stats_only.support
          << " forecast_stats_only_error=" << mx.forecast_stats_only.error
          << " forecast_stats_only_error_skill=" << mx.forecast_stats_only.error_skill
          << " forecast_linear_support=" << mx.forecast_linear.support
          << " forecast_linear_n90=" << mx.forecast_linear.n90
          << " forecast_linear_selectivity=" << mx.forecast_linear.selectivity
          << " forecast_residual_linear_support=" << mx.forecast_residual_linear.support
          << " forecast_residual_linear_error=" << mx.forecast_residual_linear.error
          << " forecast_residual_linear_error_skill="
          << mx.forecast_residual_linear.error_skill
          << " forecast_mdn_support=" << mx.forecast_mdn.support
          << " forecast_mdn_effort=" << mx.forecast_mdn.effort
          << " forecast_mdn_error=" << mx.forecast_mdn.error
          << " forecast_mdn_effort_skill=" << mx.forecast_mdn.effort_skill
          << " forecast_mdn_error_skill=" << mx.forecast_mdn.error_skill
          << " forecast_mdn_n90=" << mx.forecast_mdn.n90
          << " forecast_mdn_selectivity=" << mx.forecast_mdn.selectivity
          << " cold_start_loss=" << mx.cold_start_loss
          << " train_fit_loss=" << mx.train_fit_loss
          << " generalization_gap=" << mx.generalization_gap
          << " null_cargo=" << epoch_.null_cargo
          << " invalid_cargo=" << epoch_.invalid_cargo
          << " missing_encoding=" << epoch_.missing_encoding
          << " non_finite_encoding=" << epoch_.non_finite_encoding
          << " temporal_overlap=" << epoch_.temporal_overlap;
      return oss.str();
    }

    oss << ANSI_COLOR_Bright_Cyan << "transfer_matrix_eval.epoch"
        << ANSI_COLOR_RESET << " "
        << ANSI_COLOR_Dim_White << "(hashimyei=" << probe_hashimyei_ << ")"
        << ANSI_COLOR_RESET << "\n"
        << "\t" << ANSI_COLOR_Dim_White << "canonical_base" << ANSI_COLOR_RESET
        << ":\t" << probe_canonical_base_ << "\n"
        << "\t" << ANSI_COLOR_Dim_White << "history_file" << ANSI_COLOR_RESET
        << ":\t" << probe_metric_history_path_.string() << "\n"
        << "\t" << ANSI_COLOR_Dim_White << "epoch_count" << ANSI_COLOR_RESET
        << ":\t" << persisted_epoch_count_ << "\n";

    append_matrix_row_(&oss,
                       "throughput",
                       std::vector<report_cell_t>{
                           report_cell_t{"seen", "seen",
                                         static_cast<double>(epoch_.seen),
                                         true,
                                         true,
                                         true},
                           report_cell_t{"accepted", "accepted",
                                         static_cast<double>(epoch_.accepted),
                                         true,
                                         true,
                                         true},
                           report_cell_t{"buffered_batches", "buffered",
                                         static_cast<double>(metrics.buffered_batches),
                                         true,
                                         true,
                                         true},
                           report_cell_t{"invalid", "invalid",
                                         static_cast<double>(metrics.invalid),
                                         true,
                                         false,
                                         true},
                           report_cell_t{"train_steps", "train_steps",
                                         static_cast<double>(epoch_.train_steps),
                                         true,
                                         true,
                                         true},
                       });
    oss << "\n";

    oss << "  " << ANSI_COLOR_Dim_White << "matrix_evaluation" << ANSI_COLOR_RESET << "\n";
    const auto matrix_cells = [](const char* prefix,
                                 const row_result_t& row) {
      const std::string p(prefix);
      return std::vector<report_cell_t>{
          report_cell_t{p + "_support", "support",
                        row.support, row.has_support, true, true},
          report_cell_t{p + "_effort", "effort",
                        row.effort, row.has_effort, false, false},
          report_cell_t{p + "_error", "error",
                        row.error, row.has_error, false, false},
          report_cell_t{p + "_error_skill", "skill",
                        row.error_skill, row.has_error_skill, true, false},
          report_cell_t{p + "_n90", "n90",
                        row.n90, row.has_n90, false, false},
          report_cell_t{p + "_selectivity", "selectivity",
                        row.selectivity, row.has_selectivity, true, false},
      };
    };
    constexpr std::size_t kMethodWidth = 18;
    constexpr std::size_t kMatrixColWidth = 18;
    const auto table_header = matrix_cells("forecast_null", mx.forecast_null);
    oss << "    " << ANSI_COLOR_Dim_Cyan << std::left
        << std::setw(static_cast<int>(kMethodWidth)) << "method"
        << ANSI_COLOR_RESET;
    for (const auto& cell : table_header) {
      oss << ANSI_COLOR_Dim_Cyan << std::left
          << std::setw(static_cast<int>(kMatrixColWidth)) << cell.label
          << ANSI_COLOR_RESET;
    }
    oss << "\n";

    const auto append_method_row = [&](std::string_view method,
                                       const std::vector<report_cell_t>& cells) {
      oss << "    " << ANSI_COLOR_Dim_White << std::left
          << std::setw(static_cast<int>(kMethodWidth)) << method
          << ANSI_COLOR_RESET;
      for (const auto& cell : cells) {
        oss << format_metric_cell_(cell, kMatrixColWidth);
      }
      oss << "\n";
    };

    append_method_row("forecast.null", matrix_cells("forecast_null", mx.forecast_null));
    append_method_row("forecast.stats", matrix_cells("forecast_stats_only", mx.forecast_stats_only));
    append_method_row("forecast.linear", matrix_cells("forecast_linear", mx.forecast_linear));
    append_method_row("forecast.resid", matrix_cells("forecast_residual_linear",
                                                     mx.forecast_residual_linear));
    append_method_row("forecast.mdn", matrix_cells("forecast_mdn", mx.forecast_mdn));
    oss << "\n";

    append_matrix_row_(
        &oss,
        "health",
        std::vector<report_cell_t>{
            report_cell_t{"null_cargo", "null_cargo",
                          static_cast<double>(epoch_.null_cargo),
                          true,
                          false,
                          true},
            report_cell_t{"invalid_cargo", "invalid_cargo",
                          static_cast<double>(epoch_.invalid_cargo),
                          true,
                          false,
                          true},
            report_cell_t{"missing_encoding", "missing_encoding",
                          static_cast<double>(epoch_.missing_encoding),
                          true,
                          false,
                          true},
            report_cell_t{"non_finite_encoding", "non_finite",
                          static_cast<double>(epoch_.non_finite_encoding),
                          true,
                          false,
                          true},
            report_cell_t{"temporal_overlap", "temporal_overlap",
                          static_cast<double>(epoch_.temporal_overlap),
                          true,
                          false,
                          true},
            report_cell_t{"missing_future", "missing_future",
                          static_cast<double>(epoch_.missing_future),
                          true,
                          false,
                          true},
            report_cell_t{"future_values_present", "future_values",
                          static_cast<double>(epoch_.future_values_present),
                          true,
                          false,
                          true},
            report_cell_t{"shape_mismatch", "shape_mismatch",
                          static_cast<double>(epoch_.shape_mismatch),
                          true,
                          false,
                          true},
            report_cell_t{"optimizer_failures", "opt_fail",
                          static_cast<double>(epoch_.optimizer_failures),
                          true,
                          false,
                          true},
            report_cell_t{"cold_start_loss", "cold_start",
                          mx.cold_start_loss,
                          mx.has_cold_start_loss,
                          false,
                          false},
            report_cell_t{"train_fit_loss", "train_fit",
                          mx.train_fit_loss,
                          mx.has_train_fit_loss,
                          false,
                          false},
            report_cell_t{"generalization_gap", "gen_gap",
                          mx.generalization_gap,
                          mx.has_generalization_gap,
                          false,
                          false},
        });

    return oss.str();
  }

  void validate_wave_policy_or_throw_() const {
    if (policy_.training_window !=
        cuwacunu::camahjucunu::iitepi_wave_probe_training_window_e::
            IncomingBatch) {
      throw std::runtime_error(
          "transfer_matrix_eval unsupported wave policy TRAINING_WINDOW");
    }
    if (policy_.report_policy !=
        cuwacunu::camahjucunu::iitepi_wave_probe_report_policy_e::
            EpochEndLog) {
      throw std::runtime_error(
          "transfer_matrix_eval unsupported wave policy REPORT_POLICY");
    }
    if (policy_.objective !=
        cuwacunu::camahjucunu::iitepi_wave_probe_objective_e::
            FutureTargetDimsNll) {
      throw std::runtime_error(
          "transfer_matrix_eval unsupported wave policy OBJECTIVE");
    }
  }

  void initialize_runtime_model_or_throw_() {
    const auto contract =
        cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash_);

    model_.encoding_dims =
        contract->get<int>("VICReg", "encoding_dims");
    model_.mixture_comps = cfg_.mdn_mixture_comps;
    model_.features_hidden = cfg_.mdn_features_hidden;
    model_.residual_depth = cfg_.mdn_residual_depth;
    model_.target_dims = cfg_.mdn_target_dims.empty()
                             ? contract->get_arr<std::int64_t>(
                                   "VALUE_ESTIMATION",
                                   "target_dims",
                                   std::optional<std::vector<std::int64_t>>{
                                       std::vector<std::int64_t>{}})
                             : cfg_.mdn_target_dims;
    model_.dtype =
        cuwacunu::iitepi::config_dtype(contract_hash_, "TRANSFER_MATRIX_EVALUATION");
    model_.device =
        cuwacunu::iitepi::config_device(contract_hash_, "TRANSFER_MATRIX_EVALUATION");

    auto observation =
        cuwacunu::camahjucunu::decode_observation_spec_from_contract(contract_hash_);
    model_.channels = observation.count_channels();
    model_.horizons = observation.max_future_sequence_length();

    if (model_.encoding_dims <= 0 || model_.mixture_comps <= 0 ||
        model_.features_hidden <= 0 || model_.residual_depth < 0 ||
        model_.channels <= 0 || model_.horizons <= 0 ||
        model_.target_dims.empty()) {
      throw std::runtime_error(
          "transfer_matrix_eval invalid model specification from contract");
    }

    const auto channel_weights = observation.retrieve_channel_weights();
    if (!channel_weights.empty() &&
        static_cast<std::int64_t>(channel_weights.size()) == model_.channels) {
      channel_weights_ = torch::tensor(
          channel_weights,
          torch::TensorOptions().dtype(torch::kFloat32).device(torch::kCPU));
    }

    semantic_model_ = cuwacunu::wikimyei::mdn::MdnModel(
        model_.encoding_dims,
        static_cast<std::int64_t>(model_.target_dims.size()),
        model_.channels,
        model_.horizons,
        model_.mixture_comps,
        model_.features_hidden,
        model_.residual_depth,
        model_.dtype,
        model_.device,
        /*display_model=*/false);

    std::vector<torch::Tensor> params;
    params.reserve(64);
    for (auto& p : semantic_model_->parameters(/*recurse=*/true)) {
      if (p.requires_grad()) params.push_back(p);
    }

    if (params.empty()) {
      throw std::runtime_error(
          "transfer_matrix_eval model has no trainable parameters");
    }

    auto opts = torch::optim::AdamWOptions(cfg_.optimizer_lr)
                    .weight_decay(cfg_.optimizer_weight_decay)
                    .betas(std::make_tuple(cfg_.optimizer_beta1,
                                           cfg_.optimizer_beta2))
                    .eps(cfg_.optimizer_eps);
    optimizer_ = std::make_unique<torch::optim::AdamW>(params, opts);
  }

  void load_config_() {
    const auto contract =
        cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash_);

    cfg_.check_temporal_order = contract->get<bool>(
        "TRANSFER_MATRIX_EVALUATION",
        "check_temporal_order",
        std::optional<bool>{true});
    cfg_.validate_vicreg_out = contract->get<bool>(
        "TRANSFER_MATRIX_EVALUATION",
        "validate_vicreg_out",
        std::optional<bool>{true});
    cfg_.report_shapes = contract->get<bool>(
        "TRANSFER_MATRIX_EVALUATION",
        "report_shapes",
        std::optional<bool>{false});
    cfg_.reset_hashimyei_on_start = contract->get<bool>(
        "TRANSFER_MATRIX_EVALUATION",
        "reset_hashimyei_on_start",
        std::optional<bool>{false});

    cfg_.mdn_mixture_comps = static_cast<std::int64_t>(contract->get<int>(
        "TRANSFER_MATRIX_EVALUATION",
        "mdn_mixture_comps",
        std::optional<int>{1}));
    cfg_.mdn_features_hidden = static_cast<std::int64_t>(contract->get<int>(
        "TRANSFER_MATRIX_EVALUATION",
        "mdn_features_hidden",
        std::optional<int>{16}));
    cfg_.mdn_residual_depth = static_cast<std::int64_t>(contract->get<int>(
        "TRANSFER_MATRIX_EVALUATION",
        "mdn_residual_depth",
        std::optional<int>{0}));
    cfg_.mdn_target_dims = contract->get_arr<std::int64_t>(
        "TRANSFER_MATRIX_EVALUATION",
        "mdn_target_dims",
        std::optional<std::vector<std::int64_t>>{
            std::vector<std::int64_t>{}});

    cfg_.optimizer_lr = contract->get<double>(
        "TRANSFER_MATRIX_EVALUATION",
        "optimizer_lr",
        std::optional<double>{1e-3});
    cfg_.optimizer_weight_decay = contract->get<double>(
        "TRANSFER_MATRIX_EVALUATION",
        "optimizer_weight_decay",
        std::optional<double>{1e-2});
    cfg_.optimizer_beta1 = contract->get<double>(
        "TRANSFER_MATRIX_EVALUATION",
        "optimizer_beta1",
        std::optional<double>{0.9});
    cfg_.optimizer_beta2 = contract->get<double>(
        "TRANSFER_MATRIX_EVALUATION",
        "optimizer_beta2",
        std::optional<double>{0.999});
    cfg_.optimizer_eps = contract->get<double>(
        "TRANSFER_MATRIX_EVALUATION",
        "optimizer_eps",
        std::optional<double>{1e-8});
    cfg_.grad_clip = contract->get<double>(
        "TRANSFER_MATRIX_EVALUATION",
        "grad_clip",
        std::optional<double>{1.0});

    cfg_.nll_eps = contract->get<double>(
        "TRANSFER_MATRIX_EVALUATION",
        "nll_eps",
        std::optional<double>{1e-6});
    cfg_.nll_sigma_min = contract->get<double>(
        "TRANSFER_MATRIX_EVALUATION",
        "nll_sigma_min",
        std::optional<double>{1e-3});
    cfg_.nll_sigma_max = contract->get<double>(
        "TRANSFER_MATRIX_EVALUATION",
        "nll_sigma_max",
        std::optional<double>{0.0});

    cfg_.summary_every_steps = static_cast<std::int64_t>(contract->get<int>(
        "TRANSFER_MATRIX_EVALUATION",
        "summary_every_steps",
        std::optional<int>{64}));
    cfg_.anchor_train_ratio = contract->get<double>(
        "TRANSFER_MATRIX_EVALUATION",
        "anchor_train_ratio",
        std::optional<double>{0.70});
    cfg_.anchor_val_ratio = contract->get<double>(
        "TRANSFER_MATRIX_EVALUATION",
        "anchor_val_ratio",
        std::optional<double>{0.15});
    cfg_.anchor_test_ratio = contract->get<double>(
        "TRANSFER_MATRIX_EVALUATION",
        "anchor_test_ratio",
        std::optional<double>{0.15});
    cfg_.prequential_blocks = static_cast<std::int64_t>(contract->get<int>(
        "TRANSFER_MATRIX_EVALUATION",
        "prequential_blocks",
        std::optional<int>{4}));
    cfg_.linear_ridge_lambda = contract->get<double>(
        "TRANSFER_MATRIX_EVALUATION",
        "linear_ridge_lambda",
        std::optional<double>{1e-3});
    cfg_.gaussian_var_min = contract->get<double>(
        "TRANSFER_MATRIX_EVALUATION",
        "gaussian_var_min",
        std::optional<double>{1e-6});
    cfg_.control_shuffle_block = static_cast<std::int64_t>(contract->get<int>(
        "TRANSFER_MATRIX_EVALUATION",
        "control_shuffle_block",
        std::optional<int>{16}));
    cfg_.control_shuffle_seed = static_cast<std::int64_t>(contract->get<int>(
        "TRANSFER_MATRIX_EVALUATION",
        "control_shuffle_seed",
        std::optional<int>{1337}));

    if (cfg_.mdn_mixture_comps <= 0) cfg_.mdn_mixture_comps = 1;
    if (cfg_.mdn_features_hidden <= 0) cfg_.mdn_features_hidden = 16;
    if (cfg_.mdn_residual_depth < 0) cfg_.mdn_residual_depth = 0;

    if (cfg_.summary_every_steps < 0) cfg_.summary_every_steps = 0;
    if (cfg_.prequential_blocks < 2) cfg_.prequential_blocks = 2;
    if (cfg_.control_shuffle_block < 1) cfg_.control_shuffle_block = 1;
    if (cfg_.control_shuffle_seed < 0) cfg_.control_shuffle_seed = 0;
    if (cfg_.optimizer_lr <= 0.0) cfg_.optimizer_lr = 1e-3;
    if (cfg_.optimizer_eps <= 0.0) cfg_.optimizer_eps = 1e-8;
    if (cfg_.nll_eps <= 0.0) cfg_.nll_eps = 1e-6;
    if (cfg_.nll_sigma_min < 0.0) cfg_.nll_sigma_min = 0.0;
    if (cfg_.nll_sigma_max < 0.0) cfg_.nll_sigma_max = 0.0;
    if (cfg_.gaussian_var_min <= 0.0) cfg_.gaussian_var_min = 1e-6;
    if (cfg_.linear_ridge_lambda < 0.0) cfg_.linear_ridge_lambda = 1e-3;
    cfg_.anchor_train_ratio = std::clamp(cfg_.anchor_train_ratio, 0.0, 1.0);
    cfg_.anchor_val_ratio = std::clamp(cfg_.anchor_val_ratio, 0.0, 1.0);
    cfg_.anchor_test_ratio = std::clamp(cfg_.anchor_test_ratio, 0.0, 1.0);
    const double ratio_sum =
        cfg_.anchor_train_ratio + cfg_.anchor_val_ratio + cfg_.anchor_test_ratio;
    if (ratio_sum <= 0.0) {
      cfg_.anchor_train_ratio = 0.70;
      cfg_.anchor_val_ratio = 0.15;
      cfg_.anchor_test_ratio = 0.15;
    } else {
      cfg_.anchor_train_ratio /= ratio_sum;
      cfg_.anchor_val_ratio /= ratio_sum;
      cfg_.anchor_test_ratio /= ratio_sum;
    }
    if (cfg_.optimizer_beta1 < 0.0 || cfg_.optimizer_beta1 >= 1.0) {
      cfg_.optimizer_beta1 = 0.9;
    }
    if (cfg_.optimizer_beta2 < 0.0 || cfg_.optimizer_beta2 >= 1.0) {
      cfg_.optimizer_beta2 = 0.999;
    }
  }

  void reset_epoch_accumulators_() {
    epoch_ = epoch_stats_t{};
    epoch_matrix_ = matrix_results_t{};
    epoch_batches_.clear();
  }

  std::string contract_hash_{};
  WaveProbePolicy policy_{};
  std::string probe_hashimyei_{};
  std::string probe_canonical_base_{};
  std::filesystem::path probe_store_root_{};
  std::filesystem::path probe_artifact_directory_{};
  std::filesystem::path probe_metric_history_path_{};
  std::filesystem::path probe_anchor_split_path_{};
  std::uint64_t persisted_epoch_count_{0};
  std::unordered_map<std::string, history_metric_t> metric_history_{};
  std::unordered_map<std::string, anchor_split_e> anchor_split_manifest_{};
  bool anchor_split_dirty_{false};
  config_t cfg_{};
  model_spec_t model_{};
  epoch_stats_t epoch_{};
  matrix_results_t epoch_matrix_{};
  std::vector<train_batch_t> epoch_batches_{};

  cuwacunu::wikimyei::mdn::MdnModel semantic_model_{nullptr};
  std::unique_ptr<torch::optim::Optimizer> optimizer_{};
  torch::Tensor target_index_{};
  torch::Tensor channel_weights_{};
};

}  // namespace tsiemene
