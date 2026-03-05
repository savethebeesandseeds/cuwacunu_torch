// ./include/tsiemene/tsi.probe.representation.transfer_matrix_evaluation.h
// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#include <torch/torch.h>

#include "iitepi/contract_space_t.h"
#include "piaabo/dlogs.h"
#include "tsiemene/tsi.cargo.validation.h"
#include "tsiemene/tsi.probe.representation.h"

// ------------------------------------------------------------
// RUNTIME WARNINGS (non-blocking / informational) .h
// ------------------------------------------------------------
RUNTIME_WARNING(
    "[tsi.probe.representation.transfer_matrix_evaluation.h] Transfer-matrix probe currently performs online diagnostics and split-safety checks only; downstream supervised scoring matrix execution is planned.\n");

namespace tsiemene {

class TsiProbeRepresentationTransferMatrixEvaluation final : public TsiProbeRepresentation {
 public:
  explicit TsiProbeRepresentationTransferMatrixEvaluation(
      TsiId id,
      std::string contract_hash,
      std::string instance_name = "tsi.probe.representation.transfer_matrix_evaluation")
      : TsiProbeRepresentation(id, std::move(instance_name)),
        contract_hash_(std::move(contract_hash)) {
    load_config_();
  }

  [[nodiscard]] std::string_view type_name() const noexcept override {
    return "tsi.probe.representation.transfer_matrix_evaluation";
  }

  void step(const Wave& wave, Ingress in, BoardContext&, Emitter& out) override {
    if (in.directive != IN_STEP) return;
    if (in.signal.kind != PayloadKind::Cargo) return;

    ++stats_.seen;

    if (!in.signal.cargo) {
      ++stats_.null_cargo;
      emit_meta_(wave, out, "transfer_matrix_eval.warn reason=null-cargo");
      emit_periodic_summary_(wave, out, /*force=*/false);
      return;
    }

    const ObservationCargo& sample = *in.signal.cargo;
    bool anomaly = false;

    if (cfg_.validate_vicreg_out) {
      std::string cargo_error;
      if (!validate_observation_cargo(sample, CargoValidationStage::VicregOut,
                                      &cargo_error)) {
        ++stats_.invalid_cargo;
        anomaly = true;
        emit_meta_(wave,
                   out,
                   std::string("transfer_matrix_eval.warn reason=invalid-cargo detail=") +
                       cargo_error);
      }
    }

    const bool has_encoding = sample.encoding.defined() && sample.encoding.numel() > 0;
    if (!has_encoding) {
      ++stats_.missing_encoding;
      anomaly = true;
      emit_meta_(wave, out, "transfer_matrix_eval.warn reason=missing-encoding");
    }

    if (has_encoding && !encoding_all_finite_(sample.encoding)) {
      ++stats_.non_finite_encoding;
      anomaly = true;
      emit_meta_(wave, out, "transfer_matrix_eval.warn reason=non-finite-encoding");
    }

    if (has_future_values_(sample)) {
      ++stats_.future_values_present;
      anomaly = true;
      emit_meta_(wave, out, "transfer_matrix_eval.warn reason=future-values-present");
    }

    if (cfg_.check_temporal_order) {
      const auto order = temporal_order_ok_(sample);
      if (order.has_value() && !*order) {
        ++stats_.temporal_overlap;
        anomaly = true;
        emit_meta_(wave, out, "transfer_matrix_eval.warn reason=temporal-overlap");
      }
    }

    if (!anomaly) ++stats_.accepted;

    emit_periodic_summary_(wave, out, /*force=*/false);
    if (anomaly && cfg_.report_shapes) {
      emit_meta_(
          wave, out,
          std::string("transfer_matrix_eval.shape encoding=") +
              tensor_shape_(sample.encoding) + " past_keys=" +
              tensor_shape_(sample.past_keys) + " future_keys=" +
              tensor_shape_(sample.future_keys));
    }
  }

  void on_epoch_end(BoardContext&) override {
    // board runtime calls only with context; summary is emitted during step cadence.
  }

  void reset(BoardContext&) override {
    stats_ = stats_t{};
  }

 private:
  static constexpr std::uint64_t kSummaryEverySteps = 64;

  struct config_t {
    bool check_temporal_order{true};
    bool validate_vicreg_out{true};
    bool report_shapes{false};
  };

  struct stats_t {
    std::uint64_t seen{0};
    std::uint64_t accepted{0};
    std::uint64_t null_cargo{0};
    std::uint64_t invalid_cargo{0};
    std::uint64_t missing_encoding{0};
    std::uint64_t non_finite_encoding{0};
    std::uint64_t future_values_present{0};
    std::uint64_t temporal_overlap{0};
  };

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

  [[nodiscard]] static std::optional<double> tensor_min_as_double_(
      const torch::Tensor& t) {
    if (!t.defined() || t.numel() == 0) return std::nullopt;
    return t.to(torch::kFloat64).amin().item<double>();
  }

  [[nodiscard]] static std::optional<double> tensor_max_as_double_(
      const torch::Tensor& t) {
    if (!t.defined() || t.numel() == 0) return std::nullopt;
    return t.to(torch::kFloat64).amax().item<double>();
  }

  // Returns:
  // - nullopt when no key tensors are available for the check.
  // - true when ordering looks consistent (future strictly after past).
  // - false when overlap or inversion is detected.
  [[nodiscard]] static std::optional<bool> temporal_order_ok_(
      const ObservationCargo& sample) {
    const auto past_max = tensor_max_as_double_(sample.past_keys);
    const auto future_min = tensor_min_as_double_(sample.future_keys);
    if (!past_max.has_value() || !future_min.has_value()) return std::nullopt;
    return *future_min > *past_max;
  }

  void emit_periodic_summary_(const Wave& wave, Emitter& out, bool force) const {
    if (!force && (stats_.seen == 0 || (stats_.seen % kSummaryEverySteps) != 0)) {
      return;
    }

    std::ostringstream oss;
    oss << "transfer_matrix_eval.summary"
        << " seen=" << stats_.seen
        << " accepted=" << stats_.accepted
        << " null_cargo=" << stats_.null_cargo
        << " invalid_cargo=" << stats_.invalid_cargo
        << " missing_encoding=" << stats_.missing_encoding
        << " non_finite_encoding=" << stats_.non_finite_encoding
        << " future_values_present=" << stats_.future_values_present
        << " temporal_overlap=" << stats_.temporal_overlap;
    emit_meta_(wave, out, oss.str());
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
  }

  std::string contract_hash_{};
  config_t cfg_{};
  stats_t stats_{};
};

}  // namespace tsiemene
