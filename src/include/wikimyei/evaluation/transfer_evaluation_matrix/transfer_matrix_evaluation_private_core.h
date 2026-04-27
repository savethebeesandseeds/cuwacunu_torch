
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <limits>
#include <memory>
#include <numeric>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include <ATen/ops/linalg_pinv.h>
#include <ATen/ops/linalg_solve.h>
#include <ATen/ops/linalg_svd.h>
#include <torch/torch.h>

#include "camahjucunu/dsl/iitepi_wave/iitepi_wave.h"
#include "camahjucunu/dsl/observation_pipeline/observation_spec.h"
#include "iitepi/contract_space_t.h"
#include "piaabo/dlogs.h"
#include "piaabo/latent_lineage_state/runtime_lls.h"
#include "piaabo/torch_compat/data_analytics.h"
#include "piaabo/torch_compat/torch_utils.h"
#include "tsiemene/tsi.cargo.validation.h"
#include "tsiemene/tsi.h"
#include "tsiemene/tsi.report.h"
#include "wikimyei/inference/mdn/mixture_density_network.h"
#include "wikimyei/inference/mdn/mixture_density_network_utils.h"

DEV_WARNING("[wikimyei/evaluation/transfer_evaluation_matrix/"
            "transfer_matrix_evaluation.h] Transfer-matrix evaluation is "
            "vicreg-owned (no standalone probe hashimyei/history state). It "
            "restores matrix-evaluation training/evaluation core "
            "(null/stats/raw/embedding/residual/mdn, prequential "
            "effort/skill/n90/selectivity) and emits run-end reports.\n");

namespace cuwacunu::wikimyei::evaluation {

class VicregTransferMatrixEvaluator final {
public:
  using EvaluationPolicy =
      cuwacunu::camahjucunu::iitepi_wave_evaluation_policy_t;
  using Wave = ::tsiemene::Wave;
  using RuntimeContext = ::tsiemene::RuntimeContext;
  using Emitter = ::tsiemene::Emitter;
  using ObservationCargo = ::tsiemene::ObservationCargo;
  using CargoValidationStage = ::tsiemene::CargoValidationStage;
  using RuntimeEventKind = ::tsiemene::RuntimeEventKind;
  using RuntimeEvent = ::tsiemene::RuntimeEvent;
  using runtime_lls_document_t =
      cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_t;

  [[nodiscard]] static constexpr EvaluationPolicy default_evaluation_policy() {
    return EvaluationPolicy{
        .training_window = cuwacunu::camahjucunu::
            iitepi_wave_evaluation_training_window_e::IncomingBatch,
        .report_policy = cuwacunu::camahjucunu::
            iitepi_wave_evaluation_report_policy_e::EpochEndLog,
        .objective = cuwacunu::camahjucunu::iitepi_wave_evaluation_objective_e::
            FutureTargetFeatureIndicesNll,
    };
  }

  explicit VicregTransferMatrixEvaluator(
      std::string contract_hash,
      EvaluationPolicy policy = default_evaluation_policy())
      : contract_hash_(std::move(contract_hash)), policy_(policy) {
    load_config_();
    validate_evaluation_policy_or_throw_();
    initialize_runtime_model_or_throw_();
  }

  [[nodiscard]] std::string_view evaluator_name() const noexcept {
    return "wikimyei.evaluation.transfer_matrix_evaluation";
  }

  [[nodiscard]] static constexpr std::string_view run_schema() noexcept {
    return kRunSchema;
  }

  [[nodiscard]] static constexpr std::string_view matrix_schema() noexcept {
    return kMatrixSchema;
  }

  [[nodiscard]] const std::string &last_run_report_lls() const noexcept {
    return last_run_report_lls_;
  }

  [[nodiscard]] const std::string &last_run_matrix_report_lls() const noexcept {
    return last_run_matrix_report_lls_;
  }

  void ingest_representation_output(const Wave &wave,
                                    const ObservationCargo *sample,
                                    RuntimeContext &ctx, Emitter &out) {
    if (!ctx.debug_enabled)
      return;

    ++epoch_.seen;

    if (!sample) {
      ++epoch_.null_cargo;
      emit_meta_(wave, out, "transfer_matrix_eval.warn reason=null-cargo");
      emit_periodic_summary_(wave, out, /*force=*/false);
      return;
    }

    const ObservationCargo &cargo = *sample;
    bool diagnostics_anomaly = false;
    bool hard_invalid = false;

    if (cfg_.validate_vicreg_out) {
      std::string cargo_error;
      if (!::tsiemene::validate_observation_cargo(
              cargo, CargoValidationStage::VicregOut, &cargo_error)) {
        ++epoch_.invalid_cargo;
        diagnostics_anomaly = true;
        const bool missing_future_mask_only =
            (cargo_error == "future features/mask partial") &&
            cargo.future_features.defined() &&
            cargo.future_features.numel() > 0 &&
            (!cargo.future_mask.defined() || cargo.future_mask.numel() == 0);
        hard_invalid = !missing_future_mask_only;
        emit_meta_(
            wave, out,
            std::string(
                "transfer_matrix_eval.warn reason=invalid-cargo detail=") +
                cargo_error);
      }
    }

    const bool has_encoding =
        cargo.encoding.defined() && cargo.encoding.numel() > 0;
    if (!has_encoding) {
      ++epoch_.missing_encoding;
      diagnostics_anomaly = true;
      hard_invalid = true;
      emit_meta_(wave, out,
                 "transfer_matrix_eval.warn reason=missing-encoding");
    }

    if (has_encoding && !encoding_all_finite_(cargo.encoding)) {
      ++epoch_.non_finite_encoding;
      diagnostics_anomaly = true;
      hard_invalid = true;
      emit_meta_(wave, out,
                 "transfer_matrix_eval.warn reason=non-finite-encoding");
    }

    if (has_future_values_(cargo)) {
      ++epoch_.future_values_present;
      diagnostics_anomaly = true;
      emit_meta_(wave, out,
                 "transfer_matrix_eval.warn reason=future-values-present");
    }

    if (cfg_.check_temporal_order) {
      const auto order = temporal_order_ok_(cargo);
      if (order.has_value() && !*order) {
        ++epoch_.temporal_overlap;
        diagnostics_anomaly = true;
        hard_invalid = true;
        emit_meta_(wave, out,
                   "transfer_matrix_eval.warn reason=temporal-overlap");
      }
    }

    if (diagnostics_anomaly && cfg_.report_shapes) {
      emit_meta_(wave, out,
                 std::string("transfer_matrix_eval.shape encoding=") +
                     tensor_shape_(cargo.encoding) + " future_features=" +
                     tensor_shape_(cargo.future_features) +
                     " future_mask=" + tensor_shape_(cargo.future_mask) +
                     " past_keys=" + tensor_shape_(cargo.past_keys) +
                     " future_keys=" + tensor_shape_(cargo.future_keys));
    }

    if (hard_invalid) {
      emit_periodic_summary_(wave, out, /*force=*/false);
      return;
    }

    train_batch_t batch{};
    std::string prep_error;
    if (!prepare_train_batch_(cargo, &batch, &prep_error)) {
      emit_meta_(
          wave, out,
          std::string("transfer_matrix_eval.warn reason=train-skip detail=") +
              prep_error);
      emit_periodic_summary_(wave, out, /*force=*/false);
      return;
    }

    batch.anchor_key = derive_anchor_key_(cargo, epoch_.seen);
    ++epoch_.accepted;
    buffer_epoch_batch_(std::move(batch));

    emit_periodic_summary_(wave, out, /*force=*/false);
  }

  void handle_runtime_event(const RuntimeEvent &event, RuntimeContext &ctx,
                            Emitter &out) {
    switch (event.kind) {
    case RuntimeEventKind::RunStart:
      last_run_report_lls_.clear();
      last_run_matrix_report_lls_.clear();
      reset_epoch_accumulators_();
      return;
    case RuntimeEventKind::RunEnd:
      if (!ctx.debug_enabled) {
        last_run_report_lls_.clear();
        last_run_matrix_report_lls_.clear();
        reset_epoch_accumulators_();
        return;
      }
      if (epoch_.seen > 0 || !epoch_batches_.empty()) {
        run_epoch_training_cycle_();
        last_run_report_lls_ = build_run_report_lls_();
        last_run_matrix_report_lls_ = build_run_matrix_report_lls_();
        log_info("%s\n", build_run_report_string_(/*beautify=*/true).c_str());
        log_dbg("%s\n",
                build_run_matrix_debug_string_(/*use_color=*/true).c_str());
        if (event.wave) {
          emit_meta_(*event.wave, out,
                     std::string("transfer_matrix_eval.report_ready schema=") +
                         std::string(kRunSchema) + " matrix_schema=" +
                         std::string(kMatrixSchema) + " event=" +
                         std::string(::tsiemene::report_event_token(
                             ::tsiemene::report_event_e::RunEnd)) +
                         " accepted=" +
                         std::to_string(
                             static_cast<unsigned long long>(epoch_.accepted)));
        }
      }
      reset_epoch_accumulators_();
      return;
    default:
      return;
    }
  }

private:
  struct config_t {
    bool check_temporal_order{true};
    bool validate_vicreg_out{true};
    bool report_shapes{false};

    // Evaluator-local simple MDN architecture knobs.
    std::int64_t mdn_mixture_comps{1};
    std::int64_t mdn_features_hidden{16};
    std::int64_t mdn_residual_depth{0};
    std::vector<std::int64_t> mdn_target_feature_indices{};

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

    std::int64_t summary_every_steps{256};
  };

  struct model_spec_t {
    std::int64_t encoding_dims{0};
    std::int64_t channels{0};
    std::int64_t horizons{0};
    std::int64_t mixture_comps{0};
    std::int64_t features_hidden{0};
    std::int64_t residual_depth{0};
    std::vector<std::int64_t> target_feature_indices{};
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
    std::uint64_t train_batches_attempted{0};
    std::uint64_t train_steps{0};
    double train_ingested_positions{0.0};
    double train_ingested_support{0.0};
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
    row_result_t forecast_raw_linear{};
    row_result_t forecast_linear{};
    row_result_t forecast_residual_linear{};
    row_result_t forecast_raw_mdn{};
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
    torch::Tensor raw{};
    torch::Tensor target{};
    torch::Tensor future_mask{};
    std::string anchor_key{};
  };

  struct epoch_report_metrics_t {
    std::uint64_t invalid{0};
    std::uint64_t buffered_batches{0};
    matrix_results_t matrix{};
  };

  struct activation_report_t {
    bool valid{false};
    std::uint64_t batch_count{0};
    std::uint64_t sample_count{0};
    std::uint64_t encoding_dims{0};
    double finite_ratio{0.0};
    double mean{std::numeric_limits<double>::quiet_NaN()};
    double stddev{std::numeric_limits<double>::quiet_NaN()};
    double l1_mean_abs{std::numeric_limits<double>::quiet_NaN()};
    double l2_rms{std::numeric_limits<double>::quiet_NaN()};
    double abs_p50{std::numeric_limits<double>::quiet_NaN()};
    double abs_p90{std::numeric_limits<double>::quiet_NaN()};
    double abs_p99{std::numeric_limits<double>::quiet_NaN()};
    double per_dim_std_mean{std::numeric_limits<double>::quiet_NaN()};
    double per_dim_std_min{std::numeric_limits<double>::quiet_NaN()};
    double per_dim_std_max{std::numeric_limits<double>::quiet_NaN()};
    double dead_dim_epsilon{1e-4};
    double dead_dim_ratio{std::numeric_limits<double>::quiet_NaN()};
    double cov_trace{std::numeric_limits<double>::quiet_NaN()};
    double cov_top_eigenvalue{std::numeric_limits<double>::quiet_NaN()};
    double cov_effective_rank{std::numeric_limits<double>::quiet_NaN()};
    double cov_condition_proxy{std::numeric_limits<double>::quiet_NaN()};
  };

  struct prequential_row_t {
    std::string method{};
    std::uint64_t block_index{0};
    double prefix_support{std::numeric_limits<double>::quiet_NaN()};
    double eval_support{std::numeric_limits<double>::quiet_NaN()};
    double model_bits{std::numeric_limits<double>::quiet_NaN()};
    double null_bits{std::numeric_limits<double>::quiet_NaN()};
    double skill_bits{std::numeric_limits<double>::quiet_NaN()};
  };

  struct report_cell_t {
    std::string key{};
    const char *label{""};
    double value{std::numeric_limits<double>::quiet_NaN()};
    bool valid{false};
    bool higher_is_better{false};
    bool integer_style{false};
  };

  struct flat_dataset_t {
    torch::Tensor x{}; // [N,De]
    torch::Tensor y{}; // [N,Df]
    torch::Tensor w{}; // [N]
  };

  struct gaussian_baseline_t {
    torch::Tensor mu{}; // [Df]
    double sigma2{0.0};
    bool valid{false};
  };

  struct linear_gaussian_t {
    torch::Tensor beta{}; // [De+1,Df] (last row is bias)
    double sigma2{0.0};
    bool valid{false};
  };

  struct target_norm_t {
    torch::Tensor mean{}; // [Df] (CPU/Float64)
    torch::Tensor std{};  // [Df] (CPU/Float64)
    double logdet_bits{0.0};
    bool valid{false};
  };

  enum class feature_mode_e : std::uint8_t { Encoding = 0, Stats = 1, Raw = 2 };

  [[nodiscard]] static std::string tensor_shape_(const torch::Tensor &t) {
    if (!t.defined())
      return "undefined";
    std::ostringstream oss;
    oss << "[";
    const auto sizes = t.sizes();
    for (std::size_t i = 0; i < sizes.size(); ++i) {
      if (i > 0)
        oss << ",";
      oss << sizes[i];
    }
    oss << "]";
    return oss.str();
  }

  [[nodiscard]] static std::string tensor_signature_(const torch::Tensor &t) {
    if (!t.defined() || t.numel() == 0)
      return "undef";
    try {
      auto cpu = t.detach().to(torch::kCPU).to(torch::kFloat64).flatten();
      const auto n = cpu.numel();
      const double first = cpu[0].item<double>();
      const double last = cpu[n - 1].item<double>();
      const double mean = cpu.mean().item<double>();
      std::ostringstream oss;
      oss << "n=" << n << ",f=" << std::setprecision(17) << first
          << ",l=" << std::setprecision(17) << last
          << ",m=" << std::setprecision(17) << mean;
      return oss.str();
    } catch (...) {
      return "sig_error";
    }
  }

  [[nodiscard]] static std::string
  derive_anchor_key_(const ObservationCargo &sample, std::uint64_t seen_index) {
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

  [[nodiscard]] static bool encoding_all_finite_(const torch::Tensor &t) {
    if (!t.defined() || t.numel() == 0)
      return true;
    return torch::isfinite(t).all().item<bool>();
  }

  [[nodiscard]] static bool has_future_values_(const ObservationCargo &sample) {
    if (sample.future_mask.defined() && sample.future_mask.numel() > 0) {
      torch::Tensor m = sample.future_mask;
      if (m.dtype() != torch::kBool)
        m = m.ne(0);
      return m.any().item<bool>();
    }
    return sample.future_features.defined() &&
           sample.future_features.numel() > 0;
  }

  [[nodiscard]] static std::optional<bool>
  temporal_order_ok_(const ObservationCargo &sample) {
    auto to_bcl = [](const torch::Tensor &t) -> std::optional<torch::Tensor> {
      if (!t.defined() || t.numel() == 0)
        return std::nullopt;
      if (t.dim() == 1)
        return t.view({1, 1, t.size(0)});
      if (t.dim() == 2)
        return t.unsqueeze(0);
      if (t.dim() == 3)
        return t;
      return std::nullopt;
    };

    const auto past_opt = to_bcl(sample.past_keys);
    const auto future_opt = to_bcl(sample.future_keys);
    if (!past_opt.has_value() || !future_opt.has_value())
      return std::nullopt;

    torch::Tensor past = *past_opt;
    torch::Tensor future = *future_opt;
    if (past.size(2) <= 0 || future.size(2) <= 0)
      return std::nullopt;

    torch::Tensor past_mask;
    if (sample.mask.defined() && sample.mask.numel() > 0) {
      const auto past_mask_opt = to_bcl(sample.mask);
      if (!past_mask_opt.has_value())
        return std::nullopt;
      past_mask = past_mask_opt->ne(0);
      if (past_mask.size(2) != past.size(2))
        return std::nullopt;
    } else {
      past_mask = torch::ones_like(past, torch::kBool);
    }

    torch::Tensor future_mask;
    if (sample.future_mask.defined() && sample.future_mask.numel() > 0) {
      const auto future_mask_opt = to_bcl(sample.future_mask);
      if (!future_mask_opt.has_value())
        return std::nullopt;
      future_mask = future_mask_opt->ne(0);
      if (future_mask.size(2) != future.size(2))
        return std::nullopt;
    } else {
      future_mask = torch::ones_like(future, torch::kBool);
    }

    const auto B = std::max(past.size(0), future.size(0));
    const auto C = std::max(past.size(1), future.size(1));

    auto can_expand_to = [B, C](const torch::Tensor &t) -> bool {
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
        torch::where(future_mask, future_idx,
                     torch::full_like(future_idx, future_len))
            .amin(/*dim=*/2);
    auto future_has = future_first_idx.lt(future_len);

    auto comparable = past_has.logical_and(future_has);
    if (!comparable.any().item<bool>())
      return std::nullopt;

    auto past_last_idx_safe = past_last_idx.clamp_min(0).unsqueeze(-1);
    auto future_first_idx_safe =
        future_first_idx.clamp(0, future_len - 1).unsqueeze(-1);

    auto past_last = past.gather(/*dim=*/2, past_last_idx_safe).squeeze(-1);
    auto future_first =
        future.gather(/*dim=*/2, future_first_idx_safe).squeeze(-1);
    auto order_ok = future_first.gt(past_last);

    const bool has_violation =
        comparable.logical_and(order_ok.logical_not()).any().item<bool>();
    return !has_violation;
  }

  [[nodiscard]] torch::Tensor
  select_targets_(const torch::Tensor &future_features) {
    TORCH_CHECK(future_features.defined(),
                "[transfer_matrix_eval] future_features undefined");
    TORCH_CHECK(future_features.dim() == 4,
                "[transfer_matrix_eval] future_features must be [B,C,Hf,Dx]");
    const auto B = future_features.size(0);
    const auto C = future_features.size(1);
    const auto Hf = future_features.size(2);
    const auto D = future_features.size(3);

    TORCH_CHECK(!model_.target_feature_indices.empty(),
                "[transfer_matrix_eval] target_feature_indices is empty");
    for (const auto index : model_.target_feature_indices) {
      TORCH_CHECK(index >= 0 && index < D,
                  "[transfer_matrix_eval] target feature index out of range");
    }

    auto idx = target_index_;
    if (!idx.defined() || idx.device() != future_features.device()) {
      idx = torch::tensor(
          model_.target_feature_indices,
          torch::TensorOptions().dtype(torch::kLong).device(torch::kCPU));
      idx = idx.to(future_features.device());
      target_index_ = idx;
    }

    const auto Df =
        static_cast<std::int64_t>(model_.target_feature_indices.size());
    auto flat = future_features.reshape({B * C * Hf, D});
    auto idx2 = idx.unsqueeze(0).expand({B * C * Hf, Df});
    auto f = flat.gather(/*dim=*/1, idx2);
    return f.view({B, C, Hf, Df});
  }

  [[nodiscard]] torch::Tensor
  build_stats_features_(const ObservationCargo &sample,
                        std::int64_t expected_b) {
    if (!sample.features.defined() || sample.features.numel() == 0) {
      return torch::Tensor();
    }
    if (sample.features.dim() != 4) {
      return torch::Tensor();
    }

    auto past = sample.features.to(model_.device, model_.dtype,
                                   /*non_blocking=*/true, /*copy=*/false);
    if (past.size(0) != expected_b || past.size(0) <= 0 || past.size(1) <= 0 ||
        past.size(2) <= 0 || past.size(3) <= 0) {
      return torch::Tensor();
    }

    std::vector<std::int64_t> indices{};
    indices.reserve(model_.target_feature_indices.size());
    const auto D = past.size(3);
    for (const auto d : model_.target_feature_indices) {
      if (d >= 0 && d < D)
        indices.push_back(d);
    }
    if (indices.empty())
      return torch::Tensor();

    auto idx = torch::tensor(
        indices,
        torch::TensorOptions().dtype(torch::kLong).device(model_.device));
    auto selected = past.index_select(/*dim=*/3, idx); // [B,C,Hx,Df]

    torch::Tensor mask;
    if (sample.mask.defined() && sample.mask.numel() > 0 &&
        sample.mask.dim() == 3) {
      mask = sample.mask.to(model_.device, torch::kFloat32,
                            /*non_blocking=*/true, /*copy=*/false);
      if (mask.size(0) != selected.size(0) ||
          mask.size(1) != selected.size(1) ||
          mask.size(2) != selected.size(2)) {
        mask = torch::ones(std::vector<std::int64_t>{selected.size(0),
                                                     selected.size(1),
                                                     selected.size(2)},
                           torch::TensorOptions()
                               .device(model_.device)
                               .dtype(torch::kFloat32));
      } else {
        mask = mask.ne(0).to(torch::kFloat32);
      }
    } else {
      mask = torch::ones(
          std::vector<std::int64_t>{selected.size(0), selected.size(1),
                                    selected.size(2)},
          torch::TensorOptions().device(model_.device).dtype(torch::kFloat32));
    }

    auto w = mask.unsqueeze(-1);                  // [B,C,Hx,1]
    auto mean_num = (selected * w).sum({1, 2});   // [B,Df]
    auto mean_den = w.sum({1, 2}).clamp_min(1.0); // [B,1]
    auto mean = mean_num / mean_den;

    const auto T = selected.size(2);
    auto last_slice = selected.select(/*dim=*/2, T - 1);       // [B,C,Df]
    auto last_w = mask.select(/*dim=*/2, T - 1).unsqueeze(-1); // [B,C,1]
    auto last_num = (last_slice * last_w).sum(/*dim=*/1);      // [B,Df]
    auto last_den = last_w.sum(/*dim=*/1).clamp_min(1.0);      // [B,1]
    auto last = last_num / last_den;

    auto stats = torch::cat(std::vector<torch::Tensor>{mean, last},
                            /*dim=*/1); // [B,2*Df]
    stats =
        torch::where(torch::isfinite(stats), stats, torch::zeros_like(stats));
    return stats;
  }

  [[nodiscard]] torch::Tensor
  build_raw_features_(const ObservationCargo &sample, std::int64_t expected_b) {
    if (!sample.features.defined() || sample.features.numel() == 0) {
      return torch::Tensor();
    }
    if (sample.features.dim() != 4) {
      return torch::Tensor();
    }

    auto past = sample.features.to(model_.device, model_.dtype,
                                   /*non_blocking=*/true, /*copy=*/false);
    if (past.size(0) != expected_b || past.size(0) <= 0 || past.size(1) <= 0 ||
        past.size(2) <= 0 || past.size(3) <= 0) {
      return torch::Tensor();
    }

    torch::Tensor mask;
    if (sample.mask.defined() && sample.mask.numel() > 0 &&
        sample.mask.dim() == 3) {
      mask = sample.mask.to(model_.device, torch::kFloat32,
                            /*non_blocking=*/true, /*copy=*/false);
      if (mask.size(0) != past.size(0) || mask.size(1) != past.size(1) ||
          mask.size(2) != past.size(2)) {
        mask = torch::ones(
            std::vector<std::int64_t>{past.size(0), past.size(1), past.size(2)},
            torch::TensorOptions()
                .device(model_.device)
                .dtype(torch::kFloat32));
      } else {
        mask = mask.ne(0).to(torch::kFloat32);
      }
    } else {
      mask = torch::ones(
          std::vector<std::int64_t>{past.size(0), past.size(1), past.size(2)},
          torch::TensorOptions().device(model_.device).dtype(torch::kFloat32));
    }

    auto mask_expanded = mask.unsqueeze(-1).to(past.dtype()); // [B,C,Hx,1]
    auto masked_past =
        torch::where(mask_expanded.ne(0), past, torch::zeros_like(past));
    masked_past = torch::where(torch::isfinite(masked_past), masked_past,
                               torch::zeros_like(masked_past));

    auto raw_values = masked_past.reshape({expected_b, -1});
    auto raw_mask = mask.reshape({expected_b, -1}).to(model_.dtype);
    raw_mask = torch::where(torch::isfinite(raw_mask), raw_mask,
                            torch::zeros_like(raw_mask));

    auto raw =
        torch::cat(std::vector<torch::Tensor>{raw_values, raw_mask}, /*dim=*/1);
    raw = torch::where(torch::isfinite(raw), raw, torch::zeros_like(raw));
    return raw;
  }

  [[nodiscard]] static const torch::Tensor *
  feature_tensor_ptr_(const train_batch_t &batch, feature_mode_e mode) {
    switch (mode) {
    case feature_mode_e::Encoding:
      return &batch.encoding;
    case feature_mode_e::Stats:
      return &batch.stats;
    case feature_mode_e::Raw:
      return &batch.raw;
    default:
      return nullptr;
    }
  }

  [[nodiscard]] bool prepare_train_batch_(const ObservationCargo &sample,
                                          train_batch_t *out,
                                          std::string *error) {
    if (error)
      error->clear();
    if (!out)
      return false;

    if (!sample.future_features.defined() ||
        sample.future_features.numel() == 0) {
      ++epoch_.missing_future;
      if (error)
        *error = "future_features missing";
      return false;
    }
    if (sample.future_features.dim() != 4) {
      ++epoch_.shape_mismatch;
      if (error) {
        *error = "future_features must be [B,C,Hf,Dx], got " +
                 tensor_shape_(sample.future_features);
      }
      return false;
    }

    if (!sample.encoding.defined() || sample.encoding.numel() == 0) {
      ++epoch_.missing_encoding;
      if (error)
        *error = "encoding missing";
      return false;
    }
    if (sample.encoding.dim() != 2 && sample.encoding.dim() != 3) {
      ++epoch_.shape_mismatch;
      if (error) {
        *error = "encoding must be [B,De] or [B,Hx',De], got " +
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
        *error =
            "pooled encoding must be [B,De], got " + tensor_shape_(encoding);
      }
      return false;
    }

    if (encoding.size(1) != model_.encoding_dims) {
      ++epoch_.shape_mismatch;
      if (error) {
        *error = "encoding_dims mismatch expected=" +
                 std::to_string(model_.encoding_dims) +
                 " got=" + std::to_string(encoding.size(1));
      }
      return false;
    }

    torch::Tensor future = sample.future_features.to(
        model_.device, model_.dtype, /*non_blocking=*/true, /*copy=*/false);
    if (future.size(0) != encoding.size(0)) {
      ++epoch_.shape_mismatch;
      if (error) {
        *error =
            "batch mismatch encoding.B=" + std::to_string(encoding.size(0)) +
            " future.B=" + std::to_string(future.size(0));
      }
      return false;
    }

    if (future.size(1) != model_.channels ||
        future.size(2) != model_.horizons) {
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
      mask = sample.future_mask.to(model_.device, torch::kFloat32,
                                   /*non_blocking=*/true,
                                   /*copy=*/false);
      if (mask.size(0) != future.size(0) || mask.size(1) != future.size(1) ||
          mask.size(2) != future.size(2)) {
        ++epoch_.shape_mismatch;
        if (error) {
          *error =
              "future mask shape mismatch features=" + tensor_shape_(future) +
              " mask=" + tensor_shape_(mask);
        }
        return false;
      }
      mask = mask.ne(0).to(torch::kFloat32);
    } else {
      mask = torch::ones(
          std::vector<std::int64_t>{future.size(0), future.size(1),
                                    future.size(2)},
          torch::TensorOptions().device(model_.device).dtype(torch::kFloat32));
    }

    try {
      out->encoding = std::move(encoding);
      out->stats = build_stats_features_(sample, out->encoding.size(0));
      out->raw = build_raw_features_(sample, out->encoding.size(0));
      out->target = select_targets_(future);
      out->future_mask = std::move(mask);
    } catch (const std::exception &e) {
      ++epoch_.shape_mismatch;
      if (error)
        *error = std::string("target selection failed: ") + e.what();
      return false;
    }

    return true;
  }

  [[nodiscard]] torch::Tensor
  weighted_masked_nll_reduce_(const torch::Tensor &nll_map,
                              const torch::Tensor &mask) {
    auto weights =
        mask.defined() ? mask.to(nll_map.dtype()) : torch::ones_like(nll_map);

    if (channel_weights_.defined() &&
        channel_weights_.numel() == nll_map.size(1)) {
      auto wch = channel_weights_.to(nll_map.device(), nll_map.dtype(),
                                     /*non_blocking=*/true, /*copy=*/false);
      weights = weights * wch.view({1, nll_map.size(1), 1});
    }

    auto w_hz = torch::ones(
        std::vector<std::int64_t>{nll_map.size(2)},
        torch::TensorOptions().device(nll_map.device()).dtype(nll_map.dtype()));
    weights = weights * w_hz.view({1, 1, nll_map.size(2)});

    const auto denom = weights.sum().clamp_min(1.0);
    return (nll_map * weights).sum() / denom;
  }

  void buffer_epoch_batch_(train_batch_t &&batch) {
    train_batch_t stored{};
    stored.encoding = batch.encoding.detach().to(torch::kCPU, model_.dtype);
    if (batch.stats.defined() && batch.stats.numel() > 0) {
      stored.stats = batch.stats.detach().to(torch::kCPU, model_.dtype);
    }
    if (batch.raw.defined() && batch.raw.numel() > 0) {
      stored.raw = batch.raw.detach().to(torch::kCPU, model_.dtype);
    }
    stored.target = batch.target.detach().to(torch::kCPU, model_.dtype);
    stored.future_mask =
        batch.future_mask.detach().to(torch::kCPU, torch::kFloat32);
    stored.anchor_key = std::move(batch.anchor_key);
    epoch_batches_.push_back(std::move(stored));
  }

  [[nodiscard]] bool batch_to_runtime_(const train_batch_t &stored,
                                       train_batch_t *out,
                                       std::string *error) const {
    if (error)
      error->clear();
    if (!out)
      return false;
    if (!stored.encoding.defined() || !stored.target.defined() ||
        !stored.future_mask.defined()) {
      if (error)
        *error = "stored batch has undefined tensor(s)";
      return false;
    }
    try {
      out->encoding = stored.encoding.to(model_.device, model_.dtype,
                                         /*non_blocking=*/true, /*copy=*/false);
      if (stored.stats.defined() && stored.stats.numel() > 0) {
        out->stats = stored.stats.to(model_.device, model_.dtype,
                                     /*non_blocking=*/true, /*copy=*/false);
      } else {
        out->stats = torch::Tensor();
      }
      if (stored.raw.defined() && stored.raw.numel() > 0) {
        out->raw = stored.raw.to(model_.device, model_.dtype,
                                 /*non_blocking=*/true, /*copy=*/false);
      } else {
        out->raw = torch::Tensor();
      }
      out->target = stored.target.to(model_.device, model_.dtype,
                                     /*non_blocking=*/true, /*copy=*/false);
      out->future_mask =
          stored.future_mask.to(model_.device, torch::kFloat32,
                                /*non_blocking=*/true, /*copy=*/false);
      return true;
    } catch (const std::exception &e) {
      if (error)
        *error = e.what();
      return false;
    }
  }

  [[nodiscard]] bool
  fit_target_norm_(const std::vector<const train_batch_t *> &batches,
                   target_norm_t *out, std::string *error) const {
    if (error)
      error->clear();
    if (!out)
      return false;
    out->valid = false;
    out->mean.reset();
    out->std.reset();
    out->logdet_bits = 0.0;

    flat_dataset_t ds{};
    if (!flatten_batches_(batches, &ds)) {
      if (error)
        *error = "empty dataset for target normalization";
      return false;
    }
    if (!ds.y.defined() || !ds.w.defined() || ds.y.dim() != 2 ||
        ds.w.dim() != 1) {
      if (error)
        *error = "invalid dataset tensors for target normalization";
      return false;
    }
    try {
      auto w = ds.w.clamp_min(0.0);
      const double w_sum = w.sum().item<double>();
      if (!std::isfinite(w_sum) || w_sum <= 0.0)
        return false;
      auto w_col = w.unsqueeze(1);
      auto mean = (ds.y * w_col).sum(/*dim=*/0) / w_sum; // [Df]
      auto centered = ds.y - mean;
      auto var = ((centered * centered) * w_col).sum(/*dim=*/0) / w_sum; // [Df]
      const double eps = std::max(1e-8, cfg_.nll_eps);
      auto std = torch::sqrt(var.clamp_min(eps));
      if (!torch::isfinite(mean).all().item<bool>() ||
          !torch::isfinite(std).all().item<bool>()) {
        if (error)
          *error = "non-finite normalization statistics";
        return false;
      }
      constexpr double kInvLn2 = 1.4426950408889634074;
      const double logdet_bits = std.log().sum().item<double>() * kInvLn2;
      out->mean = mean.to(torch::kCPU, torch::kFloat64);
      out->std = std.to(torch::kCPU, torch::kFloat64);
      out->logdet_bits = std::isfinite(logdet_bits) ? logdet_bits : 0.0;
      out->valid = true;
      return true;
    } catch (const std::exception &e) {
      if (error)
        *error = e.what();
      return false;
    }
  }

  [[nodiscard]] torch::Tensor
  normalize_target_(const torch::Tensor &target,
                    const target_norm_t *norm) const {
    if (!target.defined() || !norm || !norm->valid || !norm->mean.defined() ||
        !norm->std.defined()) {
      return target;
    }
    auto mean = norm->mean.to(target.device(), target.dtype(),
                              /*non_blocking=*/true, /*copy=*/false);
    auto std = norm->std.to(target.device(), target.dtype(),
                            /*non_blocking=*/true, /*copy=*/false);
    const double eps = std::max(1e-8, cfg_.nll_eps);
    std = std.clamp_min(eps);
    return (target - mean.view({1, 1, 1, -1})) / std.view({1, 1, 1, -1});
  }

  [[nodiscard]] bool compute_batch_nll_(const train_batch_t &stored_batch,
                                        const target_norm_t *target_norm,
                                        double *out_nll, std::string *error) {
    return compute_batch_nll_for_model_(stored_batch, feature_mode_e::Encoding,
                                        target_norm, semantic_model_, out_nll,
                                        error);
  }

  [[nodiscard]] bool
  compute_batch_nll_for_model_(const train_batch_t &stored_batch,
                               feature_mode_e mode,
                               const target_norm_t *target_norm,
                               cuwacunu::wikimyei::mdn::MdnModel &model,
                               double *out_nll, std::string *error) {
    if (error)
      error->clear();
    if (!model) {
      if (error)
        *error = "evaluation model is not initialized";
      return false;
    }
    if (!out_nll)
      return false;

    train_batch_t batch{};
    if (!batch_to_runtime_(stored_batch, &batch, error)) {
      return false;
    }
    const auto *feature = feature_tensor_ptr_(batch, mode);
    if (!feature || !feature->defined() || feature->numel() == 0) {
      if (error)
        *error = "evaluation feature tensor is missing";
      return false;
    }

    try {
      torch::NoGradGuard no_grad;
      model->eval();
      auto out = model->forward_from_encoding(*feature);
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
        if (error)
          *error = "non-finite loss";
        return false;
      }
      constexpr double kInvLn2 = 1.4426950408889634074;
      *out_nll = loss.item<double>() * kInvLn2;
      if (target_norm && target_norm->valid) {
        *out_nll += target_norm->logdet_bits;
      }
      return std::isfinite(*out_nll);
    } catch (const std::exception &e) {
      if (error)
        *error = e.what();
      return false;
    }
  }

  [[nodiscard]] bool train_one_batch_(const train_batch_t &batch,
                                      const target_norm_t *target_norm,
                                      std::string *error) {
    return train_one_batch_for_model_(batch, feature_mode_e::Encoding,
                                      target_norm, semantic_model_,
                                      optimizer_.get(),
                                      /*record_training_stats=*/true, error);
  }

  [[nodiscard]] bool
  train_one_batch_for_model_(const train_batch_t &batch, feature_mode_e mode,
                             const target_norm_t *target_norm,
                             cuwacunu::wikimyei::mdn::MdnModel &model,
                             torch::optim::Optimizer *optimizer,
                             bool record_training_stats, std::string *error) {
    if (error)
      error->clear();
    if (!model || !optimizer) {
      if (error)
        *error = "evaluation model is not initialized";
      return false;
    }

    train_batch_t runtime_batch{};
    if (!batch_to_runtime_(batch, &runtime_batch, error))
      return false;
    const auto *feature = feature_tensor_ptr_(runtime_batch, mode);
    if (!feature || !feature->defined() || feature->numel() == 0) {
      if (error)
        *error = "training feature tensor is missing";
      return false;
    }

    model->train();

    try {
      optimizer->zero_grad(/*set_to_none=*/true);
      auto out = model->forward_from_encoding(*feature);
      const cuwacunu::wikimyei::mdn::MdnNllOptions nll_opt{
          cfg_.nll_eps,
          cfg_.nll_sigma_min,
          cfg_.nll_sigma_max,
      };
      auto target = normalize_target_(runtime_batch.target, target_norm);
      auto nll_map = cuwacunu::wikimyei::mdn::mdn_nll_map(
          out, target, runtime_batch.future_mask, nll_opt);
      auto loss =
          weighted_masked_nll_reduce_(nll_map, runtime_batch.future_mask);
      if (!torch::isfinite(loss).item<bool>()) {
        if (error)
          *error = "non-finite loss";
        return false;
      }

      loss.backward();
      if (cfg_.grad_clip > 0.0) {
        torch::nn::utils::clip_grad_norm_(model->parameters(), cfg_.grad_clip);
      }
      optimizer->step();

      if (record_training_stats) {
        ++epoch_.train_steps;
        try {
          const double dy = std::max<double>(
              1.0, static_cast<double>(model_.target_feature_indices.size()));
          const double positions =
              runtime_batch.future_mask.sum().item<double>();
          if (std::isfinite(positions) && positions > 0.0) {
            epoch_.train_ingested_positions += positions;
            epoch_.train_ingested_support += positions * dy;
          }
        } catch (...) {
          // Keep training behavior deterministic even if reporting accumulation
          // fails.
        }
      }
      return true;
    } catch (const std::exception &e) {
      if (error)
        *error = e.what();
      return false;
    }
  }

  void split_epoch_batches_(std::vector<const train_batch_t *> *train,
                            std::vector<const train_batch_t *> *val,
                            std::vector<const train_batch_t *> *test) {
    if (!train || !val || !test)
      return;
    train->clear();
    val->clear();
    test->clear();
    if (epoch_batches_.empty())
      return;

    struct key_group_t {
      std::string key{};
      std::vector<const train_batch_t *> batches{};
      std::uint64_t hash{0};
      anchor_split_e split{anchor_split_e::Train};
    };

    std::vector<key_group_t> groups{};
    groups.reserve(epoch_batches_.size());
    std::unordered_map<std::string, std::size_t> group_index{};
    group_index.reserve(epoch_batches_.size() * 2U);

    for (std::size_t i = 0; i < epoch_batches_.size(); ++i) {
      const auto &batch = epoch_batches_[i];
      const std::string key = batch.anchor_key.empty()
                                  ? ("batch_" + std::to_string(i))
                                  : batch.anchor_key;
      const auto it = group_index.find(key);
      if (it == group_index.end()) {
        key_group_t g{};
        g.key = key;
        g.hash = static_cast<std::uint64_t>(
            std::hash<std::string>{}(contract_hash_ + "|" + key));
        g.batches.push_back(&batch);
        const std::size_t idx = groups.size();
        groups.push_back(std::move(g));
        group_index.emplace(key, idx);
      } else {
        groups[it->second].batches.push_back(&batch);
      }
    }

    std::sort(groups.begin(), groups.end(),
              [](const key_group_t &a, const key_group_t &b) {
                if (a.hash == b.hash)
                  return a.key < b.key;
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
      n_val = std::max<std::size_t>(
          1, std::min<std::size_t>(n - 1 - n_train, n_val));
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
    const std::size_t n_test =
        (n > (n_train + n_val)) ? (n - n_train - n_val) : 0;

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

    for (const auto &g : groups) {
      auto it_manifest = anchor_split_manifest_.find(g.key);
      if (it_manifest == anchor_split_manifest_.end() ||
          it_manifest->second != g.split) {
        anchor_split_manifest_[g.key] = g.split;
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
