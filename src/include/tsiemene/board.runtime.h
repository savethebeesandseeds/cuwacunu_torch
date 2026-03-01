// ./include/tsiemene/board.runtime.h
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

#include "tsiemene/board.contract.circuit.h"

namespace tsiemene {

struct Event {
  Tsi* tsi{};
  Wave wave{};
  Ingress in{};
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
  CircuitEmitter(const CompiledCircuit& cc, std::deque<Event>& q) : cc_(cc), q_(q) {}

  // Set by runtime before calling tsi.step()
  void set_source(Tsi* s) noexcept {
    src_ = s;
    emits_this_step_ = 0;
  }

  void trace_step(const Event& e) {
    std::ostringstream oss;
    const DirectiveSpec* in_spec = find_directive(*e.tsi, e.in.directive, DirectiveDir::In);
    oss << "step tsi=" << e.tsi->instance_name()
        << " in=[" << e.in.directive << (in_spec ? kind_token(in_spec->kind.kind) : ":unknown") << "]"
        << " signal={" << summarize_signal_(e.in.signal) << "}"
        << " directives={" << summarize_directives_(*e.tsi) << "}";
    emit_meta_(e.wave, oss.str());
  }

  void trace_step_done(const Event& e) {
    std::ostringstream oss;
    oss << "step.done tsi=" << e.tsi->instance_name()
        << " emits=" << emits_this_step_
        << " queue=" << q_.size();
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
        q_.push_back(Event{
            .tsi = t.tsi,
            .wave = wave,
            .in = Ingress{.directive = t.directive, .signal = out}, // Signal copy (Tensor is cheap)
        });
        routed = true;
        if (!is_meta) ++emits_this_step_;
      }
    }

    if (!routed && !is_meta) {
      const DirectiveSpec* out_spec = find_directive(*src_, out_directive, DirectiveDir::Out);
      std::ostringstream oss;
      oss << "drop from=" << src_->instance_name()
          << "[" << out_directive << (out_spec ? kind_token(out_spec->kind.kind) : ":unknown") << "]"
          << " signal={" << summarize_signal_(out) << "} no_route";
      emit_meta_(wave, oss.str());
    }
  }

 private:
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
  Tsi* src_{nullptr};
  std::uint64_t emits_this_step_{0};
  bool in_meta_emit_{false};
};

inline std::uint64_t run_wave_compiled(const CompiledCircuit& cc, Wave w0, Ingress start, BoardContext& ctx) {
  (void)ctx;
  if (!cc.start_tsi) return 0;
  std::deque<Event> q;
  q.push_back(Event{.tsi = cc.start_tsi, .wave = w0, .in = std::move(start)});

  CircuitEmitter emitter(cc, q);

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
      emitter.trace_step(e);
      e.tsi->step(e.wave, std::move(e.in), ctx, emitter);
      emitter.trace_step_done(e);
      ++steps;
      if (e.tsi == cc.start_tsi) continuation_wave = e.wave;
    }

    if (!cc.start_tsi) break;
    if (!cc.start_tsi->requests_runtime_continuation()) break;
    if (continuation_steps >= kMaxContinuationSteps) break;

    ++continuation_steps;
    Ingress follow = cc.start_tsi->runtime_continuation_ingress();
    continuation_wave = advance_wave_batch(continuation_wave);
    q.push_back(Event{
        .tsi = cc.start_tsi,
        .wave = continuation_wave,
        .in = std::move(follow),
    });
  }
  return steps;
}

inline std::uint64_t run_wave(const Circuit& c, Wave w0, Ingress start, BoardContext& ctx) {
  CompiledCircuit cc{};
  if (!compile_circuit(c, &cc, nullptr)) return 0;
  return run_wave_compiled(cc, w0, std::move(start), ctx);
}

} // namespace tsiemene
