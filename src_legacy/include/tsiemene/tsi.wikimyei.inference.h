// ./include/tsiemene/tsi.wikimyei.inference.h
// SPDX-License-Identifier: MIT
#pragma once

#include "tsiemene/tsi.wikimyei.h"

namespace tsiemene {

class TsiWikimyeiInference : public TsiWikimyei {
public:
  virtual ~TsiWikimyeiInference() = default;
  [[nodiscard]] virtual bool emits_expectation_directive() const noexcept {
    return false;
  }
  [[nodiscard]] virtual bool emits_loss_directive() const noexcept {
    return false;
  }
};

} // namespace tsiemene
