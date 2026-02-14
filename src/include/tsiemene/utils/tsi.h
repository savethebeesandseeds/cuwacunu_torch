// ./include/tsiemene/tsi.h
// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <span>
#include <string_view>

#include "tsiemene/ports.h"
#include "tsiemene/waves.h"

namespace tsiemene {

using TsiId = std::uint64_t;

// Opaque runtime context (board/session can hang whatever it wants here).
struct TsiContext {
  void* user = nullptr;
};

// One ingress token delivered to one input port.
struct Ingress {
  PortId port{};
  Signal signal{};
};

// Output interface. The board/runtime owns routing + broadcasting.
//
// Contract: emit() either works, or it is a wave-stopping failure handled
// by the board (exceptions / abort / error-channel).
class Emitter {
 public:
  virtual ~Emitter() = default;
  virtual void emit(const Wave& wave, PortId out_port, Signal out) = 0;

  // Convenience sugar.
  void emit_tensor(const Wave& w, PortId out_port, torch::Tensor t) {
    emit(w, out_port, tensor_signal(std::move(t)));
  }
  void emit_string(const Wave& w, PortId out_port, std::string s) {
    emit(w, out_port, string_signal(std::move(s)));
  }
};

// Optional trait (board can use this for “Kahn-proof eligible” checks).
enum class Determinism : std::uint8_t {
  Deterministic,
  SeededStochastic,
  Nondeterministic,
};

// A TSI is a step-driven process.
// The runtime delivers an ingress token (wave + port + signal),
// and the TSI emits zero or more output tokens.
//
// NOTE on parallelism:
// - If the board schedules step() concurrently (across ports or instances),
//   the TSI must be thread-safe (or the board must serialize calls).
class Tsi {
 public:
  virtual ~Tsi() = default;

  [[nodiscard]] virtual std::string_view type_name() const noexcept = 0;
  [[nodiscard]] virtual std::string_view instance_name() const noexcept = 0;
  [[nodiscard]] virtual TsiId id() const noexcept = 0;

  [[nodiscard]] virtual std::span<const Port> ports() const noexcept = 0;

  [[nodiscard]] virtual bool is_sink() const noexcept { return false; }
  [[nodiscard]] virtual Determinism determinism() const noexcept { return Determinism::Deterministic; }

  // One step: respond to one ingress token.
  virtual void step(const Wave& wave, Ingress in, TsiContext& ctx, Emitter& out) = 0;

  virtual void reset(TsiContext& /*ctx*/) {}
};

class TsiSink : public Tsi {
 public:
  [[nodiscard]] bool is_sink() const noexcept override { return true; }
};

} // namespace tsiemene
