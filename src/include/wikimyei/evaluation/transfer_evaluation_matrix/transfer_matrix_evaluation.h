// ./include/wikimyei/evaluation/transfer_evaluation_matrix/transfer_matrix_evaluation.h
// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

#include <torch/torch.h>

#include "camahjucunu/dsl/iitepi_wave/iitepi_wave.h"
#include "iitepi/contract_space_t.h"
#include "piaabo/dlogs.h"
#include "tsiemene/tsi.h"
#include "tsiemene/tsi.cargo.validation.h"
#include "tsiemene/tsi.report.h"

DEV_WARNING(
    "[wikimyei/evaluation/transfer_evaluation_matrix/transfer_matrix_evaluation.h] Transfer-matrix evaluation now runs as an ephemeral report engine owned by vicreg runtime: no standalone probe identity, no persisted history files, and no hashimyei-scoped state.\n");

namespace cuwacunu::wikimyei::evaluation {

class TransferMatrixEvaluationReport final {
 public:
  using WaveProbePolicy = cuwacunu::camahjucunu::iitepi_wave_probe_policy_t;
  static constexpr tsiemene::DirectiveId IN_STEP = tsiemene::directive_id::Step;
  static constexpr const char* kContractSection =
      "WIKIMYEI_INFERENCE_TRANSFER_MATRIX_EVALUATION";

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

  explicit TransferMatrixEvaluationReport(
      std::string contract_hash,
      WaveProbePolicy policy = default_wave_policy())
      : contract_hash_(std::move(contract_hash)),
        policy_(policy) {
    validate_wave_policy_or_throw_();
    load_config_();
  }

  [[nodiscard]] std::string_view type_name() const noexcept {
    return "wikimyei.evaluation.transfer_matrix_evaluation";
  }

  [[nodiscard]] static constexpr std::string_view run_schema() noexcept {
    return kRunSchema;
  }

  [[nodiscard]] const std::string& last_run_report_lls() const noexcept {
    return last_run_report_lls_;
  }

  void step(const tsiemene::Wave& wave,
            tsiemene::Ingress in,
            tsiemene::BoardContext& ctx,
            tsiemene::Emitter& out) {
    (void)wave;
    (void)out;
    if (in.directive != IN_STEP) return;
    if (in.signal.kind != tsiemene::PayloadKind::Cargo) return;
    if (!ctx.debug_enabled) return;

    ++epoch_.seen;

    if (!in.signal.cargo) {
      ++epoch_.null_cargo;
      return;
    }

    const tsiemene::ObservationCargo& sample = *in.signal.cargo;
    bool hard_invalid = false;

    if (cfg_.validate_vicreg_out) {
      std::string cargo_error;
      if (!tsiemene::validate_observation_cargo(
              sample, tsiemene::CargoValidationStage::VicregOut,
              &cargo_error)) {
        ++epoch_.invalid_cargo;
        hard_invalid = true;
      }
    }

    const bool has_encoding =
        sample.encoding.defined() && sample.encoding.numel() > 0;
    if (!has_encoding) {
      ++epoch_.missing_encoding;
      hard_invalid = true;
    }

    if (has_encoding && !encoding_all_finite_(sample.encoding)) {
      ++epoch_.non_finite_encoding;
      hard_invalid = true;
    }

    if (has_future_values_(sample)) {
      ++epoch_.future_values_present;
    }

    if (cfg_.check_temporal_order) {
      const auto order_ok = temporal_order_ok_(sample);
      if (order_ok.has_value() && !*order_ok) {
        ++epoch_.temporal_overlap;
        hard_invalid = true;
      }
    }

    if (hard_invalid) return;

    ++epoch_.accepted;
    accumulate_encoding_stats_(sample.encoding);
    accumulate_future_support_(sample);
  }

  tsiemene::RuntimeEventAction on_event(const tsiemene::RuntimeEvent& event,
                                        tsiemene::BoardContext& ctx,
                                        tsiemene::Emitter& out) {
    switch (event.kind) {
      case tsiemene::RuntimeEventKind::RunStart:
        last_run_report_lls_.clear();
        reset_epoch_();
        return tsiemene::RuntimeEventAction{};
      case tsiemene::RuntimeEventKind::RunEnd:
        if (has_pending_epoch_state_()) {
          finalize_run_report_(ctx.debug_enabled, event.wave, &out);
        }
        return tsiemene::RuntimeEventAction{};
      default:
        break;
    }
    return tsiemene::RuntimeEventAction{};
  }

 private:
  struct config_t {
    bool check_temporal_order{true};
    bool validate_vicreg_out{true};
    bool report_shapes{false};
    std::int64_t summary_every_steps{64};
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

    std::uint64_t encoding_tensor_count{0};
    std::uint64_t encoding_total_elements{0};
    std::uint64_t encoding_finite_elements{0};
    double encoding_sum{0.0};
    double encoding_sq_sum{0.0};
    double encoding_abs_max{0.0};
    std::int64_t encoding_last_dim{0};

    std::uint64_t future_mask_elements{0};
    std::uint64_t future_mask_active{0};
  };

  [[nodiscard]] bool has_pending_epoch_state_() const {
    return epoch_.seen > 0 || epoch_.encoding_tensor_count > 0;
  }

  void reset_epoch_() { epoch_ = epoch_stats_t{}; }

  void finalize_run_report_(bool debug_enabled,
                            const tsiemene::Wave* wave,
                            tsiemene::Emitter* out) {
    if (!debug_enabled) {
      last_run_report_lls_.clear();
      reset_epoch_();
      return;
    }

    last_run_report_lls_ = build_run_report_lls_();

    log_info(
        "[transfer_matrix_eval] run_end flush seen=%llu accepted=%llu\n",
        static_cast<unsigned long long>(epoch_.seen),
        static_cast<unsigned long long>(epoch_.accepted));
    log_info("%s\n", build_run_report_string_(/*beautify=*/true).c_str());
    if (wave && out) {
      emit_meta_(*wave,
                 *out,
                 std::string("transfer_matrix_eval.report_ready schema=") +
                     kRunSchema +
                     " event=" +
                     std::string(
                         tsiemene::report_event_token(
                             tsiemene::report_event_e::RunEnd)) +
                     " accepted=" +
                     std::to_string(static_cast<unsigned long long>(
                         epoch_.accepted)));
    }

    reset_epoch_();
  }

  [[nodiscard]] static bool encoding_all_finite_(const torch::Tensor& t) {
    if (!t.defined() || t.numel() == 0) return true;
    return torch::isfinite(t).all().item<bool>();
  }

  [[nodiscard]] static bool has_future_values_(
      const tsiemene::ObservationCargo& sample) {
    if (sample.future_mask.defined() && sample.future_mask.numel() > 0) {
      torch::Tensor m = sample.future_mask;
      if (m.dtype() != torch::kBool) m = m.ne(0);
      return m.any().item<bool>();
    }
    return sample.future_features.defined() && sample.future_features.numel() > 0;
  }

  [[nodiscard]] static std::optional<bool> temporal_order_ok_(
      const tsiemene::ObservationCargo& sample) {
    const auto to_bcl = [](const torch::Tensor& t) -> std::optional<torch::Tensor> {
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

    const auto past_last =
        past.select(2, past.size(2) - 1).to(torch::kCPU).to(torch::kFloat64);
    const auto future_first = future.select(2, 0).to(torch::kCPU).to(torch::kFloat64);

    const auto pa = past_last.reshape({-1});
    const auto fb = future_first.reshape({-1});
    const auto n = std::min<std::int64_t>(pa.size(0), fb.size(0));
    if (n <= 0) return std::nullopt;

    auto lhs = pa.narrow(0, 0, n);
    auto rhs = fb.narrow(0, 0, n);
    return (lhs.lt(rhs)).all().item<bool>();
  }

  void accumulate_encoding_stats_(const torch::Tensor& encoding) {
    if (!encoding.defined() || encoding.numel() <= 0) return;
    ++epoch_.encoding_tensor_count;
    epoch_.encoding_last_dim = encoding.dim() > 0 ? encoding.size(encoding.dim() - 1)
                                                  : 0;
    auto flat = encoding.detach().to(torch::kCPU).to(torch::kFloat64).reshape({-1});
    const auto total = static_cast<std::uint64_t>(flat.numel());
    epoch_.encoding_total_elements += total;

    auto finite = torch::isfinite(flat);
    const auto finite_count =
        static_cast<std::uint64_t>(finite.sum().item<std::int64_t>());
    epoch_.encoding_finite_elements += finite_count;
    if (finite_count == 0) return;

    auto vals = flat.index({finite});
    epoch_.encoding_sum += vals.sum().item<double>();
    epoch_.encoding_sq_sum += (vals * vals).sum().item<double>();

    const double abs_max = vals.abs().max().item<double>();
    if (std::isfinite(abs_max)) {
      epoch_.encoding_abs_max = std::max(epoch_.encoding_abs_max, abs_max);
    }
  }

  void accumulate_future_support_(const tsiemene::ObservationCargo& sample) {
    if (!sample.future_mask.defined() || sample.future_mask.numel() <= 0) {
      return;
    }
    auto m = sample.future_mask;
    if (m.dtype() != torch::kBool) m = m.ne(0);
    m = m.to(torch::kCPU);

    epoch_.future_mask_elements += static_cast<std::uint64_t>(m.numel());
    epoch_.future_mask_active +=
        static_cast<std::uint64_t>(m.sum().item<std::int64_t>());
  }

  [[nodiscard]] std::string build_run_report_lls_() const {
    const double finite_ratio =
        epoch_.encoding_total_elements > 0
            ? static_cast<double>(epoch_.encoding_finite_elements) /
                  static_cast<double>(epoch_.encoding_total_elements)
            : 0.0;

    const double mean = epoch_.encoding_finite_elements > 0
                            ? epoch_.encoding_sum /
                                  static_cast<double>(epoch_.encoding_finite_elements)
                            : std::numeric_limits<double>::quiet_NaN();
    const double var = epoch_.encoding_finite_elements > 0
                           ? (epoch_.encoding_sq_sum /
                                  static_cast<double>(epoch_.encoding_finite_elements)) -
                                 (mean * mean)
                           : std::numeric_limits<double>::quiet_NaN();
    const double stddev = (std::isfinite(var) && var > 0.0) ? std::sqrt(var) : 0.0;

    const double future_support_ratio =
        epoch_.future_mask_elements > 0
            ? static_cast<double>(epoch_.future_mask_active) /
                  static_cast<double>(epoch_.future_mask_elements)
            : 0.0;

    std::ostringstream out;
    out.setf(std::ios::fixed);
    out.precision(12);
    out << "schema=" << kRunSchema << "\n";
    out << "contract_hash=" << contract_hash_ << "\n";
    out << "seen=" << epoch_.seen << "\n";
    out << "accepted=" << epoch_.accepted << "\n";
    out << "invalid_cargo=" << epoch_.invalid_cargo << "\n";
    out << "null_cargo=" << epoch_.null_cargo << "\n";
    out << "missing_encoding=" << epoch_.missing_encoding << "\n";
    out << "non_finite_encoding=" << epoch_.non_finite_encoding << "\n";
    out << "temporal_overlap=" << epoch_.temporal_overlap << "\n";
    out << "future_values_present=" << epoch_.future_values_present << "\n";

    out << "encoding.tensor_count=" << epoch_.encoding_tensor_count << "\n";
    out << "encoding.last_dim=" << epoch_.encoding_last_dim << "\n";
    out << "encoding.total_elements=" << epoch_.encoding_total_elements << "\n";
    out << "encoding.finite_elements=" << epoch_.encoding_finite_elements << "\n";
    out << "encoding.finite_ratio=" << finite_ratio << "\n";
    out << "encoding.mean=" << mean << "\n";
    out << "encoding.stddev=" << stddev << "\n";
    out << "encoding.abs_max=" << epoch_.encoding_abs_max << "\n";

    out << "future.mask_elements=" << epoch_.future_mask_elements << "\n";
    out << "future.mask_active=" << epoch_.future_mask_active << "\n";
    out << "future.support_ratio=" << future_support_ratio << "\n";
    return out.str();
  }

  [[nodiscard]] std::string build_run_report_string_(bool beautify) const {
    if (!beautify) return build_run_report_lls_();

    std::ostringstream out;
    out << "\033[96mtransfer_matrix_eval.run\033[0m\n";
    out << "\tcontract_hash: " << contract_hash_ << "\n";
    out << "\tschema: " << kRunSchema << "\n";
    out << "\tseen=" << epoch_.seen << " accepted=" << epoch_.accepted
        << " invalid=" << epoch_.invalid_cargo
        << " null_cargo=" << epoch_.null_cargo
        << " missing_encoding=" << epoch_.missing_encoding
        << " non_finite=" << epoch_.non_finite_encoding
        << " temporal_overlap=" << epoch_.temporal_overlap << "\n";
    out << "\tencoding.tensor_count=" << epoch_.encoding_tensor_count
        << " last_dim=" << epoch_.encoding_last_dim
        << " finite=" << epoch_.encoding_finite_elements << "/"
        << epoch_.encoding_total_elements
        << " abs_max=" << epoch_.encoding_abs_max << "\n";
    out << "\tfuture.mask_active=" << epoch_.future_mask_active << "/"
        << epoch_.future_mask_elements
        << " future_values_present=" << epoch_.future_values_present << "\n";
    return out.str();
  }

  void emit_meta_(const tsiemene::Wave& wave,
                  tsiemene::Emitter& out,
                  std::string msg) const {
    out.emit_string(wave, tsiemene::directive_id::Meta, std::move(msg));
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

  void load_config_() {
    try {
      const auto contract =
          cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash_);
      if (!contract) return;

      cfg_.check_temporal_order = contract->template get<bool>(
          kContractSection, "check_temporal_order",
          std::optional<bool>{true});
      cfg_.validate_vicreg_out = contract->template get<bool>(
          kContractSection, "validate_vicreg_out",
          std::optional<bool>{true});
      cfg_.report_shapes = contract->template get<bool>(
          kContractSection, "report_shapes",
          std::optional<bool>{false});
      cfg_.summary_every_steps = static_cast<std::int64_t>(contract->template get<int>(
          kContractSection, "summary_every_steps",
          std::optional<int>{64}));
      if (cfg_.summary_every_steps < 0) cfg_.summary_every_steps = 0;
    } catch (const std::exception& e) {
      log_warn("[transfer_matrix_eval] config load fallback to defaults: %s\n",
               e.what());
    } catch (...) {
      log_warn(
          "[transfer_matrix_eval] config load fallback to defaults: unknown error\n");
    }
  }

  static constexpr const char* kRunSchema =
      "tsi.wikimyei.representation.vicreg.transfer_matrix_evaluation.run.v1";

  std::string contract_hash_{};
  WaveProbePolicy policy_{};
  config_t cfg_{};
  epoch_stats_t epoch_{};
  std::string last_run_report_lls_{};
};

}  // namespace cuwacunu::wikimyei::evaluation
