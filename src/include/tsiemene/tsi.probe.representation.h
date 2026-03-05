// ./include/tsiemene/tsi.probe.representation.h
// SPDX-License-Identifier: MIT
#pragma once

#include <string>
#include <string_view>

#include "tsiemene/tsi.probe.h"

namespace tsiemene {

class TsiProbeRepresentation : public TsiProbe {
 public:
  static constexpr DirectiveId IN_STEP  = directive_id::Step;
  static constexpr DirectiveId OUT_META = directive_id::Meta;

  [[nodiscard]] std::string_view instance_name() const noexcept override { return instance_name_; }
  [[nodiscard]] TsiId id() const noexcept override { return id_; }

  [[nodiscard]] std::span<const DirectiveSpec> directives() const noexcept override {
    static constexpr DirectiveSpec kDirectives[] = {
      directive(IN_STEP, DirectiveDir::In, KindSpec::Cargo(), "probe ingress cargo"),
      directive(OUT_META, DirectiveDir::Out, KindSpec::String(), "runtime trace/meta stream"),
    };
    return std::span<const DirectiveSpec>(kDirectives, 2);
  }

 protected:
  explicit TsiProbeRepresentation(TsiId id,
                                  std::string instance_name)
      : id_(id),
        instance_name_(std::move(instance_name)) {}

  void emit_meta_(const Wave& wave, Emitter& out, std::string msg) const {
    out.emit_string(wave, OUT_META, std::move(msg));
  }

 private:
  TsiId id_{};
  std::string instance_name_;
};

} // namespace tsiemene
