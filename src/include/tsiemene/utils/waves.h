// ./include/tsiemene/waves.h
// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <string>

#include <torch/torch.h>

#include "tsiemene/utils/ports.h"

namespace tsiemene {

using WaveId = std::uint64_t;

// This is the wave identity carried through the whole experiment,
// plus an item index (the “within-wave” sequence id).
//
// In practice:
// - dataloader emits items with {wave.id, wave.i + k}
// - downstream TSIs preserve the same {id,i} for causality tracking
struct Wave {
  WaveId id{};
  std::uint64_t i{}; // within-wave item id
};

// Runtime signal.
// Minimal: either Tensor or String.
struct Signal {
  PayloadKind kind{PayloadKind::Tensor};
  torch::Tensor tensor{};
  std::string text{};
};

[[nodiscard]] inline Signal tensor_signal(torch::Tensor t) {
  return Signal{.kind = PayloadKind::Tensor, .tensor = std::move(t), .text = {}};
}

[[nodiscard]] inline Signal string_signal(std::string s) {
  return Signal{.kind = PayloadKind::String, .tensor = {}, .text = std::move(s)};
}

} // namespace tsiemene
