// ./include/tsiemene/tsi.cargo.validation.h
// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include <torch/torch.h>

#include "camahjucunu/data/observation_sample.h"
#include "iitepi/runtime_binding/runtime_binding.wave.h"

namespace tsiemene {

enum class CargoValidationStage : std::uint8_t {
  SourceOut = 0,
  VicregIn = 1,
  VicregOut = 2,
  ExpectedValueIn = 3,
  ExpectedValueTrainIn = 4,
};

[[nodiscard]] inline std::string_view
cargo_stage_token(CargoValidationStage stage) noexcept {
  switch (stage) {
  case CargoValidationStage::SourceOut:
    return "source.out";
  case CargoValidationStage::VicregIn:
    return "vicreg.in";
  case CargoValidationStage::VicregOut:
    return "vicreg.out";
  case CargoValidationStage::ExpectedValueIn:
    return "expected_value.in";
  case CargoValidationStage::ExpectedValueTrainIn:
    return "expected_value.train.in";
  }
  return "unknown";
}

[[nodiscard]] inline bool
validate_feature_mask_pair_(const torch::Tensor &features,
                            const torch::Tensor &mask, std::string_view name,
                            std::string *error) {
  if (!features.defined()) {
    if (error)
      *error = std::string(name) + ".features undefined";
    return false;
  }
  if (!mask.defined()) {
    if (error)
      *error = std::string(name) + ".mask undefined";
    return false;
  }
  if (features.dim() < 2) {
    if (error)
      *error = std::string(name) + ".features dim<2";
    return false;
  }
  if (mask.dim() + 1 != features.dim()) {
    if (error) {
      *error = std::string(name) +
               ".dim mismatch features=" + std::to_string(features.dim()) +
               " mask=" + std::to_string(mask.dim());
    }
    return false;
  }
  for (std::int64_t i = 0; i < mask.dim(); ++i) {
    if (mask.size(i) != features.size(i)) {
      if (error) {
        *error = std::string(name) +
                 ".shape mismatch dim=" + std::to_string(i) +
                 " features=" + std::to_string(features.size(i)) +
                 " mask=" + std::to_string(mask.size(i));
      }
      return false;
    }
  }
  return true;
}

[[nodiscard]] inline bool
validate_observation_cargo(const ObservationCargo &sample,
                           CargoValidationStage stage,
                           std::string *error = nullptr) {
  if (error)
    error->clear();

  if (!validate_feature_mask_pair_(sample.features, sample.mask, "past",
                                   error)) {
    return false;
  }

  const bool requires_future =
      (stage == CargoValidationStage::ExpectedValueTrainIn);
  if (requires_future &&
      (!sample.future_features.defined() || !sample.future_mask.defined())) {
    if (error)
      *error = "future features/mask required for train-enabled ExpectedValue";
    return false;
  }
  if (sample.future_features.defined() != sample.future_mask.defined()) {
    if (error)
      *error = "future features/mask partial";
    return false;
  }
  if (sample.future_features.defined() &&
      !validate_feature_mask_pair_(sample.future_features, sample.future_mask,
                                   "future", error)) {
    return false;
  }

  if (stage == CargoValidationStage::VicregOut ||
      stage == CargoValidationStage::ExpectedValueIn ||
      stage == CargoValidationStage::ExpectedValueTrainIn) {
    if (!sample.encoding.defined()) {
      if (error)
        *error = "encoding undefined";
      return false;
    }
    if (sample.features.dim() >= 4 && sample.encoding.dim() >= 1 &&
        sample.encoding.size(0) != sample.features.size(0)) {
      if (error) {
        *error = "encoding batch mismatch features.B=" +
                 std::to_string(sample.features.size(0)) +
                 " encoding.B=" + std::to_string(sample.encoding.size(0));
      }
      return false;
    }
  }

  return true;
}

[[nodiscard]] inline bool
validate_observation_cargo(const ObservationCargoPtr &sample,
                           CargoValidationStage stage,
                           std::string *error = nullptr) {
  if (error)
    error->clear();
  if (!sample) {
    if (error)
      *error = "cargo pointer null";
    return false;
  }
  return validate_observation_cargo(*sample, stage, error);
}

} // namespace tsiemene
