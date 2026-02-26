// ./include/tsiemene/tsi.wikimyei.representation.h
// SPDX-License-Identifier: MIT
#pragma once

#include "tsiemene/tsi.wikimyei.h"

namespace tsiemene {

class TsiWikimyeiRepresentation : public TsiWikimyei {
 public:
  virtual ~TsiWikimyeiRepresentation() = default;
  [[nodiscard]] virtual bool emits_loss_directive() const noexcept { return false; }
  [[nodiscard]] virtual bool supports_jkimyei_facet() const noexcept { return false; }
};

} // namespace tsiemene
