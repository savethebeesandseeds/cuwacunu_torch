// ./include/tsiemene/runtime.h
// SPDX-License-Identifier: MIT
#pragma once

#include <deque>

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
  void set_source(const Tsi* s) noexcept { src_ = s; }

  void emit(const Wave& wave, PortId out_port, Signal out) override {
    // route to all hops whose "from" matches (broadcast if multiple)
    bool routed = false;
    for (std::size_t i = 0; i < c_.hop_count; ++i) {
      const Hop& h = c_.hops[i];
      if (h.from.tsi == src_ && h.from.port == out_port) {
        q_.push_back(Event{
          .tsi = h.to.tsi,
          .wave = wave,
          .in = Ingress{.port = h.to.port, .signal = out} // Signal copy (Tensor is cheap)
        });
        routed = true;
      }
    }
    (void)routed; // later you may assert/log if !routed
  }

 private:
  const Circuit& c_;
  std::deque<Event>& q_;
  const Tsi* src_{nullptr};
};

inline std::uint64_t run_wave(const Circuit& c, Wave w0, Ingress start, TsiContext& ctx) {
  std::deque<Event> q;
  q.push_back(Event{.tsi = c.hops[0].from.tsi, .wave = w0, .in = std::move(start)});

  CircuitEmitter emitter(c, q);

  std::uint64_t steps = 0;
  while (!q.empty()) {
    Event e = std::move(q.front());
    q.pop_front();
    if (!e.tsi) continue;

    emitter.set_source(e.tsi);
    const_cast<Tsi*>(e.tsi)->step(e.wave, std::move(e.in), ctx, emitter);
    ++steps;
  }
  return steps;
}

} // namespace tsiemene
