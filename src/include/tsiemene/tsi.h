// ./include/tsiemene/tsi.h
// SPDX-License-Identifier: MIT
#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

#include "tsiemene/tsi.directive.registry.h"
#include "tsiemene/tsi.domain.h"
#include "iitepi/board/board.wave.h"

namespace tsiemene {

using TsiId = std::uint64_t;
class Tsi;

// Opaque runtime context (board/session can hang whatever it wants here).
struct BoardContext {
  void* user = nullptr;
  std::uint64_t wave_mode_flags{0};
  bool debug_enabled{false};
  std::string run_id{};
};

// One ingress token delivered to one input directive.
struct Ingress {
  DirectiveId directive{};
  Signal signal{};
};

// Output interface. The board/runtime owns routing + broadcasting.
class Emitter {
 public:
  virtual ~Emitter() = default;
  virtual void emit(const Wave& wave, DirectiveId out_directive, Signal out) = 0;

  void emit_tensor(const Wave& w, DirectiveId out_directive, torch::Tensor t) {
    emit(w, out_directive, tensor_signal(std::move(t)));
  }
  void emit_string(const Wave& w, DirectiveId out_directive, std::string s) {
    emit(w, out_directive, string_signal(std::move(s)));
  }
  void emit_cargo(const Wave& w, DirectiveId out_directive, ObservationCargoPtr c) {
    emit(w, out_directive, cargo_signal(std::move(c)));
  }
};

enum class Determinism : std::uint8_t {
  Deterministic,
  SeededStochastic,
  Nondeterministic,
};

enum class RuntimeEventKind : std::uint8_t {
  RunStart = 0,
  RunEnd = 1,
  EpochStart = 2,
  EpochEnd = 3,
  QueueDrained = 4,
  StepStart = 5,
  StepDone = 6,
  Route = 7,
  Drop = 8,
  Backpressure = 9,
};

struct RuntimeEvent {
  RuntimeEventKind kind{RuntimeEventKind::StepStart};
  const Wave* wave{nullptr};
  const Ingress* ingress{nullptr};
  const Signal* signal{nullptr};

  Tsi* source{nullptr};
  Tsi* target{nullptr};
  DirectiveId out_directive{};
  DirectiveId in_directive{};

  std::uint64_t step_count{0};
  std::size_t queue_size{0};
  std::size_t max_queue_size{0};
  std::size_t dropped_by_backpressure{0};
};

struct RuntimeEventAction {
  bool request_continuation{false};
  Ingress continuation_ingress{};
};

// A TSI is a step-driven process.
class Tsi {
 public:
  virtual ~Tsi() = default;

  [[nodiscard]] virtual std::string_view type_name() const noexcept = 0;
  [[nodiscard]] virtual std::string_view instance_name() const noexcept = 0;
  [[nodiscard]] virtual TsiId id() const noexcept = 0;
  [[nodiscard]] virtual TsiDomain domain() const noexcept = 0;

  [[nodiscard]] virtual std::span<const DirectiveSpec> directives() const noexcept = 0;

  [[nodiscard]] const DirectiveSpec* find_directive(DirectiveId id,
                                                    DirectiveDir dir) const noexcept {
    for (const DirectiveSpec& d : directives()) {
      if (d.id == id && d.dir == dir) return &d;
    }
    return nullptr;
  }

  [[nodiscard]] bool has_input(DirectiveId in_directive,
                               PayloadKind expected_kind) const noexcept {
    const auto* spec = find_directive(in_directive, DirectiveDir::In);
    return spec && spec->kind.kind == expected_kind;
  }

  [[nodiscard]] bool has_output(DirectiveId out_directive,
                                PayloadKind expected_kind) const noexcept {
    const auto* spec = find_directive(out_directive, DirectiveDir::Out);
    return spec && spec->kind.kind == expected_kind;
  }

  // Hop compatibility hook used by board/circuit validation:
  // target input directive must accept source outgoing kind.
  // Default policy is strict kind equality; subclasses may override.
  [[nodiscard]] virtual bool is_compatible(DirectiveId target_incoming_directive,
                                           PayloadKind source_outgoing_kind) const noexcept {
    return has_input(target_incoming_directive, source_outgoing_kind);
  }

  [[nodiscard]] virtual bool is_sink() const noexcept { return false; }
  [[nodiscard]] virtual Determinism determinism() const noexcept { return Determinism::Deterministic; }

  // Layer contract hooks used by circuit validation.
  [[nodiscard]] virtual bool can_be_circuit_root() const noexcept { return true; }
  [[nodiscard]] virtual bool can_be_circuit_terminal() const noexcept { return is_sink(); }
  [[nodiscard]] virtual bool allows_hop_to(const Tsi& downstream,
                                           DirectiveId out_directive,
                                           DirectiveId in_directive) const noexcept {
    (void)downstream;
    (void)out_directive;
    (void)in_directive;
    return true;
  }
  [[nodiscard]] virtual bool allows_hop_from(const Tsi& upstream,
                                             DirectiveId out_directive,
                                             DirectiveId in_directive) const noexcept {
    (void)upstream;
    (void)out_directive;
    (void)in_directive;
    return true;
  }

  // Returns true when runtime should not auto-emit @meta to avoid feedback loops.
  [[nodiscard]] virtual bool suppress_runtime_meta_feedback() const noexcept { return false; }

  virtual void step(const Wave& wave, Ingress in, BoardContext& ctx, Emitter& out) = 0;
  virtual RuntimeEventAction on_event(const RuntimeEvent&,
                                      BoardContext&,
                                      Emitter&) {
    return RuntimeEventAction{};
  }
};

} // namespace tsiemene
