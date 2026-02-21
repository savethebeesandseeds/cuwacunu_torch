// ./include/tsiemene/utils/runtime.h
// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <deque>
#include <sstream>
#include <string>
#include <string_view>

#include "tsiemene/utils/circuits.h"

namespace tsiemene {

struct Event {
  const Tsi* tsi{};
  Wave wave{};
  Ingress in{};
};

class CircuitEmitter final : public Emitter {
 public:
  CircuitEmitter(const Circuit& c, std::deque<Event>& q) : c_(c), q_(q) {}

  // Set by runtime before calling tsi.step()
  void set_source(const Tsi* s) noexcept {
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

    // route to all hops whose "from" matches (broadcast if multiple)
    bool routed = false;
    for (std::size_t i = 0; i < c_.hop_count; ++i) {
      const Hop& h = c_.hops[i];
      if (h.from.tsi == src_ && h.from.directive == out_directive) {
        if (!is_meta) {
          const DirectiveSpec* out_spec = find_directive(*src_, out_directive, DirectiveDir::Out);
          const DirectiveSpec* in_spec  = find_directive(*h.to.tsi, h.to.directive, DirectiveDir::In);
          std::ostringstream oss;
          oss << "route from=" << src_->instance_name()
              << "[" << out_directive << (out_spec ? kind_token(out_spec->kind.kind) : ":unknown") << "]"
              << " to=" << h.to.tsi->instance_name()
              << "[" << h.to.directive << (in_spec ? kind_token(in_spec->kind.kind) : ":unknown") << "]"
              << " signal={" << summarize_signal_(out) << "}";
          emit_meta_(wave, oss.str());
        }
        q_.push_back(Event{
          .tsi = h.to.tsi,
          .wave = wave,
          .in = Ingress{.directive = h.to.directive, .signal = out} // Signal copy (Tensor is cheap)
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
    if (src_->type_name() == "tsi.sink.log.sys") return; // avoid feedback loops
    if (!find_directive(*src_, directive_id::Meta, DirectiveDir::Out)) return;
    if (in_meta_emit_) return;

    in_meta_emit_ = true;
    emit(wave, directive_id::Meta, string_signal(std::move(msg)));
    in_meta_emit_ = false;
  }

  const Circuit& c_;
  std::deque<Event>& q_;
  const Tsi* src_{nullptr};
  std::uint64_t emits_this_step_{0};
  bool in_meta_emit_{false};
};

inline std::uint64_t run_wave(const Circuit& c, Wave w0, Ingress start, TsiContext& ctx) {
  (void)ctx;
  if (c.hop_count == 0 || !c.hops || !c.hops[0].from.tsi) return 0;
  std::deque<Event> q;
  q.push_back(Event{.tsi = c.hops[0].from.tsi, .wave = w0, .in = std::move(start)});

  CircuitEmitter emitter(c, q);

  std::uint64_t steps = 0;
  while (!q.empty()) {
    Event e = std::move(q.front());
    q.pop_front();
    if (!e.tsi) continue;

    emitter.set_source(e.tsi);
    emitter.trace_step(e);
    const_cast<Tsi*>(e.tsi)->step(e.wave, std::move(e.in), ctx, emitter);
    emitter.trace_step_done(e);
    ++steps;
  }
  return steps;
}

} // namespace tsiemene
