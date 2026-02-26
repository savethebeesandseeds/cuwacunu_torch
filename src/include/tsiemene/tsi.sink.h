// ./include/tsiemene/tsi.sink.h
// SPDX-License-Identifier: MIT
#pragma once

#include "tsiemene/tsi.h"

namespace tsiemene {

class TsiSink : public Tsi {
 public:
  [[nodiscard]] TsiDomain domain() const noexcept final { return TsiDomain::Sink; }
  [[nodiscard]] bool is_sink() const noexcept final { return true; }
  [[nodiscard]] bool can_be_circuit_root() const noexcept override { return false; }
  [[nodiscard]] bool can_be_circuit_terminal() const noexcept override { return true; }
  [[nodiscard]] bool allows_hop_to(const Tsi& downstream,
                                   DirectiveId out_directive,
                                   DirectiveId in_directive) const noexcept override {
    (void)out_directive;
    (void)in_directive;
    return downstream.domain() == TsiDomain::Sink;
  }
};

} // namespace tsiemene
