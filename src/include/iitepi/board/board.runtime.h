// ./include/iitepi/board.runtime.h
// SPDX-License-Identifier: MIT
#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "piaabo/dlogs.h"
#include "iitepi/board/board.contract.circuit.h"

namespace tsiemene {

struct Event {
  Tsi* tsi{};
  Wave wave{};
  Ingress in{};
};

enum class RuntimeTraceLevel : std::uint8_t {
  Off = 0,
  Step = 1,
  Verbose = 2,
};

struct RuntimeFlowControl {
  // Off: disable runtime trace meta emitted by board runtime.
  // Step: step/step.done trace only.
  // Verbose: step trace plus per-route and drop trace.
  RuntimeTraceLevel trace_level{RuntimeTraceLevel::Verbose};
  // 0 means unbounded queue growth.
  std::size_t max_queue_size{0};
};

struct RouteKey {
  Tsi* from{};
  DirectiveId directive{};

  [[nodiscard]] bool operator==(const RouteKey& other) const noexcept {
    return from == other.from && directive == other.directive;
  }
};

struct RouteKeyHash {
  [[nodiscard]] std::size_t operator()(const RouteKey& key) const noexcept {
    const std::size_t a = std::hash<const void*>{}(static_cast<const void*>(key.from));
    const std::size_t b = std::hash<std::string_view>{}(key.directive);
    return a ^ (b + 0x9e3779b97f4a7c15ULL + (a << 6) + (a >> 2));
  }
};

struct RouteTarget {
  Tsi* tsi{};
  DirectiveId directive{};
};

struct CompiledCircuit {
  std::string_view doc{};
  Tsi* start_tsi{};
  std::size_t hop_count{};
  std::unordered_map<RouteKey, std::vector<RouteTarget>, RouteKeyHash> routes{};
};

[[nodiscard]] inline std::size_t hash_combine(std::size_t seed, std::size_t value) noexcept {
  return seed ^ (value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2));
}

[[nodiscard]] inline std::size_t circuit_topology_signature(const Circuit& c) noexcept {
  std::size_t sig = c.hop_count;
  if (!c.hops || c.hop_count == 0) return sig;
  for (std::size_t i = 0; i < c.hop_count; ++i) {
    const Hop& h = c.hops[i];
    sig = hash_combine(sig, std::hash<const void*>{}(static_cast<const void*>(h.from.tsi)));
    sig = hash_combine(sig, std::hash<std::string_view>{}(h.from.directive));
    sig = hash_combine(sig, std::hash<const void*>{}(static_cast<const void*>(h.to.tsi)));
    sig = hash_combine(sig, std::hash<std::string_view>{}(h.to.directive));
  }
  return sig;
}

[[nodiscard]] inline bool compile_circuit(const Circuit& c,
                                          CompiledCircuit* out,
                                          CircuitIssue* issue = nullptr) noexcept {
  if (!out) return false;
  *out = CompiledCircuit{};
  out->doc = c.doc;
  out->hop_count = c.hop_count;

  if (c.hop_count == 0) {
    if (issue) { issue->what = "empty circuit"; issue->hop_index = 0; }
    return false;
  }
  if (!c.hops) {
    if (issue) { issue->what = "null hops pointer"; issue->hop_index = 0; }
    return false;
  }
  if (!c.hops[0].from.tsi) {
    if (issue) { issue->what = "null tsi pointer"; issue->hop_index = 0; }
    return false;
  }

  out->start_tsi = c.hops[0].from.tsi;
  out->routes.reserve(c.hop_count);

  for (std::size_t i = 0; i < c.hop_count; ++i) {
    const Hop& h = c.hops[i];
    if (!h.from.tsi || !h.to.tsi) {
      if (issue) { issue->what = "null tsi pointer"; issue->hop_index = i; }
      return false;
    }

    RouteKey key{.from = h.from.tsi, .directive = h.from.directive};
    auto& fanout = out->routes[key];
    fanout.push_back(RouteTarget{
        .tsi = h.to.tsi,
        .directive = h.to.directive,
    });
  }

  return true;
}

class CircuitEmitter final : public Emitter {
 public:
  CircuitEmitter(const CompiledCircuit& cc,
                 std::deque<Event>& q,
                 RuntimeFlowControl flow,
                 BoardContext& ctx)
      : cc_(cc), q_(q), flow_(flow), ctx_(ctx) {}

  // Set by runtime before calling tsi.step()
  void set_source(Tsi* s) noexcept {
    src_ = s;
    emits_this_step_ = 0;
  }

  [[nodiscard]] RuntimeEventAction dispatch_runtime_event(
      Tsi* receiver,
      const RuntimeEvent& event) {
    return dispatch_runtime_event_(receiver, event);
  }

  void trace_step(const Event& e, std::uint64_t step_count) {
    RuntimeEvent event{};
    event.kind = RuntimeEventKind::StepStart;
    event.wave = &e.wave;
    event.ingress = &e.in;
    event.source = e.tsi;
    event.target = e.tsi;
    event.step_count = step_count;
    event.queue_size = q_.size();
    event.max_queue_size = flow_.max_queue_size;
    event.dropped_by_backpressure = backpressure_dropped_;
    (void)dispatch_runtime_event_(e.tsi, event);

    if (!trace_step_enabled_()) return;
    std::ostringstream oss;
    const DirectiveSpec* in_spec = find_directive(*e.tsi, e.in.directive, DirectiveDir::In);
    oss << "step tsi=" << e.tsi->instance_name()
        << " in=[" << e.in.directive << (in_spec ? kind_token(in_spec->kind.kind) : ":unknown") << "]"
        << " signal={" << summarize_signal_(e.in.signal) << "}"
        << " directives={" << summarize_directives_(*e.tsi) << "}";
    emit_meta_(e.wave, oss.str());
  }

  void trace_step_done(const Event& e, std::uint64_t step_count) {
    RuntimeEvent event{};
    event.kind = RuntimeEventKind::StepDone;
    event.wave = &e.wave;
    event.ingress = &e.in;
    event.source = e.tsi;
    event.target = e.tsi;
    event.step_count = step_count;
    event.queue_size = q_.size();
    event.max_queue_size = flow_.max_queue_size;
    event.dropped_by_backpressure = backpressure_dropped_;
    (void)dispatch_runtime_event_(e.tsi, event);

    if (!trace_step_enabled_()) return;
    std::ostringstream oss;
    oss << "step.done tsi=" << e.tsi->instance_name()
        << " emits=" << emits_this_step_
        << " queue=" << q_.size();
    if (backpressure_dropped_ > 0) {
      oss << " dropped_bp=" << backpressure_dropped_;
    }
    emit_meta_(e.wave, oss.str());
  }

  void emit(const Wave& wave, DirectiveId out_directive, Signal out) override {
    if (!src_) return;
    const bool is_meta = (out_directive == directive_id::Meta);

    // Route to all indexed targets for this (tsi, directive) pair.
    bool routed = false;
    const RouteKey key{.from = src_, .directive = out_directive};
    const auto it = cc_.routes.find(key);
    if (it != cc_.routes.end()) {
      for (const RouteTarget& t : it->second) {
        if (!is_meta) {
          RuntimeEvent route_event{};
          route_event.kind = RuntimeEventKind::Route;
          route_event.wave = &wave;
          route_event.signal = &out;
          route_event.source = src_;
          route_event.target = t.tsi;
          route_event.out_directive = out_directive;
          route_event.in_directive = t.directive;
          route_event.queue_size = q_.size();
          route_event.max_queue_size = flow_.max_queue_size;
          route_event.dropped_by_backpressure = backpressure_dropped_;
          (void)dispatch_runtime_event_(src_, route_event);

          if (trace_verbose_enabled_()) {
            const DirectiveSpec* out_spec = find_directive(*src_, out_directive, DirectiveDir::Out);
            const DirectiveSpec* in_spec = find_directive(*t.tsi, t.directive, DirectiveDir::In);
            std::ostringstream oss;
            oss << "route from=" << src_->instance_name()
                << "[" << out_directive << (out_spec ? kind_token(out_spec->kind.kind) : ":unknown") << "]"
                << " to=" << t.tsi->instance_name()
                << "[" << t.directive << (in_spec ? kind_token(in_spec->kind.kind) : ":unknown") << "]"
                << " signal={" << summarize_signal_(out) << "}";
            emit_meta_(wave, oss.str());
          }
        }
        Event ev{
            .tsi = t.tsi,
            .wave = wave,
            .in = Ingress{.directive = t.directive, .signal = out}, // Signal copy (Tensor is cheap)
        };
        const bool queued = enqueue_with_backpressure_(wave, std::move(ev), out_directive, t, out, is_meta);
        routed = true;
        if (queued && !is_meta) ++emits_this_step_;
        if (halted_by_backpressure_) break;
      }
    }

    if (!routed && !is_meta) {
      RuntimeEvent drop_event{};
      drop_event.kind = RuntimeEventKind::Drop;
      drop_event.wave = &wave;
      drop_event.signal = &out;
      drop_event.source = src_;
      drop_event.out_directive = out_directive;
      drop_event.queue_size = q_.size();
      drop_event.max_queue_size = flow_.max_queue_size;
      drop_event.dropped_by_backpressure = backpressure_dropped_;
      (void)dispatch_runtime_event_(src_, drop_event);

      if (trace_verbose_enabled_()) {
        const DirectiveSpec* out_spec =
            find_directive(*src_, out_directive, DirectiveDir::Out);
        std::ostringstream oss;
        oss << "drop from=" << src_->instance_name()
            << "[" << out_directive
            << (out_spec ? kind_token(out_spec->kind.kind) : ":unknown")
            << "]"
            << " signal={" << summarize_signal_(out) << "} no_route";
        emit_meta_(wave, oss.str());
      }
    }
  }

  [[nodiscard]] bool halted_by_backpressure() const noexcept {
    return halted_by_backpressure_;
  }
  [[nodiscard]] std::size_t dropped_by_backpressure() const noexcept {
    return backpressure_dropped_;
  }

 private:
  [[nodiscard]] bool trace_step_enabled_() const noexcept {
    return static_cast<int>(flow_.trace_level) >=
           static_cast<int>(RuntimeTraceLevel::Step);
  }
  [[nodiscard]] bool trace_verbose_enabled_() const noexcept {
    return static_cast<int>(flow_.trace_level) >=
           static_cast<int>(RuntimeTraceLevel::Verbose);
  }

  bool enqueue_with_backpressure_(const Wave& wave,
                                  Event ev,
                                  DirectiveId out_directive,
                                  const RouteTarget& target,
                                  const Signal& out,
                                  bool is_meta) {
    if (flow_.max_queue_size > 0 && q_.size() >= flow_.max_queue_size) {
      ++backpressure_dropped_;
      if (!is_meta) {
        RuntimeEvent backpressure_event{};
        backpressure_event.kind = RuntimeEventKind::Backpressure;
        backpressure_event.wave = &wave;
        backpressure_event.signal = &out;
        backpressure_event.source = src_;
        backpressure_event.target = target.tsi;
        backpressure_event.out_directive = out_directive;
        backpressure_event.in_directive = target.directive;
        backpressure_event.queue_size = q_.size();
        backpressure_event.max_queue_size = flow_.max_queue_size;
        backpressure_event.dropped_by_backpressure = backpressure_dropped_;
        (void)dispatch_runtime_event_(src_, backpressure_event);
      }
      if (!warned_backpressure_) {
        warned_backpressure_ = true;
        log_warn(
            "[tsiemene.runtime] queue backpressure reached limit=%zu policy=halt\n",
            flow_.max_queue_size);
      }
      if (!is_meta && trace_verbose_enabled_()) {
        const DirectiveSpec* out_spec =
            find_directive(*src_, out_directive, DirectiveDir::Out);
        const DirectiveSpec* in_spec =
            find_directive(*target.tsi, target.directive, DirectiveDir::In);
        std::ostringstream oss;
        oss << "drop from=" << src_->instance_name()
            << "[" << out_directive
            << (out_spec ? kind_token(out_spec->kind.kind) : ":unknown") << "]"
            << " to=" << target.tsi->instance_name()
            << "[" << target.directive
            << (in_spec ? kind_token(in_spec->kind.kind) : ":unknown") << "]"
            << " signal={" << summarize_signal_(out)
            << "} reason=queue_backpressure limit=" << flow_.max_queue_size;
        emit_meta_(wave, oss.str());
      }
      halted_by_backpressure_ = true;
      return false;
    }
    q_.push_back(std::move(ev));
    return true;
  }

  [[nodiscard]] RuntimeEventAction dispatch_runtime_event_(
      Tsi* receiver,
      const RuntimeEvent& event) {
    if (!receiver) return RuntimeEventAction{};
    if (in_event_callback_) return RuntimeEventAction{};
    Tsi* const saved_src = src_;
    const std::uint64_t saved_emits = emits_this_step_;
    src_ = receiver;
    in_event_callback_ = true;
    const RuntimeEventAction action = receiver->on_event(event, ctx_, *this);
    in_event_callback_ = false;
    src_ = saved_src;
    emits_this_step_ = saved_emits;
    return action;
  }

  static std::string summarize_signal_(const Signal& s) {
    if (s.kind == PayloadKind::String) {
      constexpr std::size_t kPreview = 48;
      std::ostringstream oss;
      oss << ":str bytes=" << s.text.size();
      if (s.text.empty()) return oss.str();
      oss << " text=\"";
      if (s.text.size() <= kPreview) {
        oss << s.text;
      } else {
        oss << s.text.substr(0, kPreview) << "...";
      }
      oss << "\"";
      return oss.str();
    }
    if (s.kind == PayloadKind::Cargo) {
      std::ostringstream oss;
      oss << ":cargo ";
      if (!s.cargo) {
        oss << "null";
      } else {
        oss << "ptr=" << static_cast<const void*>(s.cargo.get());
      }
      return oss.str();
    }
    if (!s.tensor.defined()) {
      return ":tensor undefined";
    }
    std::ostringstream oss;
    oss << ":tensor shape=[";
    const auto sizes = s.tensor.sizes();
    for (std::size_t i = 0; i < sizes.size(); ++i) {
      if (i > 0) oss << ",";
      oss << sizes[i];
    }
    oss << "]";
    return oss.str();
  }

  static std::string summarize_directives_(const Tsi& t) {
    std::ostringstream oss;
    bool first = true;
    for (const DirectiveSpec& d : t.directives()) {
      if (!first) oss << ",";
      oss << (is_in(d.dir) ? "in" : "out")
          << "[" << d.id << kind_token(d.kind.kind) << "]";
      first = false;
    }
    return oss.str();
  }

  void emit_meta_(const Wave& wave, std::string msg) {
    if (!src_) return;
    if (src_->suppress_runtime_meta_feedback()) return;
    if (!find_directive(*src_, directive_id::Meta, DirectiveDir::Out)) return;
    if (in_meta_emit_) return;

    in_meta_emit_ = true;
    emit(wave, directive_id::Meta, string_signal(std::move(msg)));
    in_meta_emit_ = false;
  }

  const CompiledCircuit& cc_;
  std::deque<Event>& q_;
  RuntimeFlowControl flow_{};
  BoardContext& ctx_;
  Tsi* src_{nullptr};
  std::uint64_t emits_this_step_{0};
  bool in_meta_emit_{false};
  bool in_event_callback_{false};
  bool halted_by_backpressure_{false};
  bool warned_backpressure_{false};
  std::size_t backpressure_dropped_{0};
};

inline std::uint64_t run_wave_compiled(const CompiledCircuit& cc,
                                       Wave w0,
                                       Ingress start,
                                       BoardContext& ctx,
                                       RuntimeFlowControl flow = {}) {
  if (!cc.start_tsi) return 0;
  std::deque<Event> q;
  q.push_back(Event{.tsi = cc.start_tsi, .wave = w0, .in = std::move(start)});

  CircuitEmitter emitter(cc, q, flow, ctx);

  std::uint64_t steps = 0;
  Wave continuation_wave = w0;
  std::uint64_t continuation_steps = 0;
  constexpr std::uint64_t kMaxContinuationSteps = 1000000ULL;

  while (true) {
    while (!q.empty()) {
      Event e = std::move(q.front());
      q.pop_front();
      if (!e.tsi) continue;

      emitter.set_source(e.tsi);
      emitter.trace_step(e, steps);
      e.tsi->step(e.wave, std::move(e.in), ctx, emitter);
      emitter.trace_step_done(e, steps + 1);
      ++steps;
      if (e.tsi == cc.start_tsi) continuation_wave = e.wave;
      if (emitter.halted_by_backpressure()) break;
    }
    if (emitter.halted_by_backpressure()) break;

    emitter.set_source(cc.start_tsi);
    RuntimeEvent queue_drained_event{};
    queue_drained_event.kind = RuntimeEventKind::QueueDrained;
    queue_drained_event.wave = &continuation_wave;
    queue_drained_event.source = cc.start_tsi;
    queue_drained_event.target = cc.start_tsi;
    queue_drained_event.step_count = steps;
    queue_drained_event.queue_size = q.size();
    queue_drained_event.max_queue_size = flow.max_queue_size;
    queue_drained_event.dropped_by_backpressure =
        emitter.dropped_by_backpressure();
    const RuntimeEventAction action =
        emitter.dispatch_runtime_event(cc.start_tsi, queue_drained_event);
    if (!action.request_continuation) break;
    if (continuation_steps >= kMaxContinuationSteps) break;

    ++continuation_steps;
    continuation_wave = advance_wave_batch(continuation_wave);
    q.push_back(Event{
        .tsi = cc.start_tsi,
        .wave = continuation_wave,
        .in = action.continuation_ingress,
    });
  }
  if (emitter.halted_by_backpressure()) {
    log_warn(
        "[tsiemene.runtime] execution halted due to queue backpressure (limit=%zu, dropped=%zu)\n",
        flow.max_queue_size,
        emitter.dropped_by_backpressure());
  }
  return steps;
}

inline std::uint64_t run_wave(const Circuit& c, Wave w0, Ingress start, BoardContext& ctx) {
  CompiledCircuit cc{};
  if (!compile_circuit(c, &cc, nullptr)) return 0;
  return run_wave_compiled(cc, w0, std::move(start), ctx);
}

} // namespace tsiemene
