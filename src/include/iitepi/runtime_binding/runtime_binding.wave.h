// ./include/iitepi/runtime_binding/runtime_binding.wave.h
// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>

#include <torch/torch.h>

#include "tsiemene/tsi.directive.registry.h"

namespace cuwacunu {
namespace camahjucunu {
namespace data {
struct observation_sample_t;
}  // namespace data
}  // namespace camahjucunu
}  // namespace cuwacunu

namespace tsiemene {

using ObservationCargo = cuwacunu::camahjucunu::data::observation_sample_t;
using ObservationCargoPtr = std::shared_ptr<ObservationCargo>;

using WaveId = std::uint64_t;

// Wave dispatch cursor carried through the whole circuit execution.
//
// Core fields:
// - id: runtime/circuit wave stream id
// - episode: outer episode index (run_contract maps epoch loop into this)
// - batch: batch index inside the current episode
// - i: monotonic event index in this wave stream
struct WaveCursor {
  WaveId id{};
  std::uint64_t i{};
  std::uint64_t episode{};
  std::uint64_t batch{};
};

// Wave execution state carried through the whole circuit execution.
//
// Optional generic time-span:
// - when has_time_span=true, [span_begin_ms, span_end_ms] can be consumed
//   by source nodes that support range dispatch.
struct Wave {
  WaveCursor cursor{};
  std::uint64_t max_batches_per_epoch{};
  std::int64_t span_begin_ms{};
  std::int64_t span_end_ms{};
  bool has_time_span{false};
};

[[nodiscard]] inline Wave advance_wave_batch(Wave w) noexcept {
  ++w.cursor.i;
  ++w.cursor.batch;
  return w;
}

[[nodiscard]] inline Wave normalize_wave_span(Wave w) noexcept {
  if (!w.has_time_span) return w;
  if (w.span_begin_ms > w.span_end_ms) std::swap(w.span_begin_ms, w.span_end_ms);
  return w;
}

// Runtime signal transport payload.
struct Signal {
  PayloadKind kind{PayloadKind::Tensor};
  torch::Tensor tensor{};
  std::string text{};
  ObservationCargoPtr cargo{};
};

[[nodiscard]] inline Signal tensor_signal(torch::Tensor t) {
  return Signal{
      .kind = PayloadKind::Tensor,
      .tensor = std::move(t),
      .text = {},
      .cargo = {}};
}

[[nodiscard]] inline Signal string_signal(std::string s) {
  return Signal{
      .kind = PayloadKind::String,
      .tensor = {},
      .text = std::move(s),
      .cargo = {}};
}

[[nodiscard]] inline Signal cargo_signal(ObservationCargoPtr c) {
  return Signal{
      .kind = PayloadKind::Cargo,
      .tensor = {},
      .text = {},
      .cargo = std::move(c)};
}

} // namespace tsiemene
