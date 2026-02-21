// ./include/tsiemene/tsi.wikimyei.wave.generator.h
// SPDX-License-Identifier: MIT
#pragma once

#include <string>
#include <string_view>

#include "tsiemene/utils/tsi.h"

namespace tsiemene {

class TsiWaveGenerator final : public Tsi {
 public:
  static constexpr DirectiveId IN_PAYLOAD  = directive_id::Payload;
  static constexpr DirectiveId OUT_PAYLOAD = directive_id::Payload;
  static constexpr DirectiveId OUT_META    = directive_id::Meta;

  explicit TsiWaveGenerator(TsiId id,
                            std::string instance_name = "tsi.wikimyei.wave.generator")
      : id_(id), instance_name_(std::move(instance_name)) {}

  [[nodiscard]] std::string_view type_name() const noexcept override { return "tsi.wikimyei.wave.generator"; }
  [[nodiscard]] std::string_view instance_name() const noexcept override { return instance_name_; }
  [[nodiscard]] TsiId id() const noexcept override { return id_; }

  [[nodiscard]] std::span<const DirectiveSpec> directives() const noexcept override {
    static constexpr DirectiveSpec kDirectives[] = {
      directive(IN_PAYLOAD, DirectiveDir::In, KindSpec::String(), "generator command string"),
      directive(OUT_PAYLOAD, DirectiveDir::Out, KindSpec::String(), "wave payload string"),
      directive(OUT_META, DirectiveDir::Out, KindSpec::String(), "runtime trace/meta stream"),
    };
    return std::span<const DirectiveSpec>(kDirectives, 3);
  }

  void step(const Wave& wave, Ingress in, TsiContext&, Emitter& out) override {
    if (in.directive != IN_PAYLOAD) return;
    if (in.signal.kind != PayloadKind::String) return;
    out.emit_string(wave, OUT_PAYLOAD, std::move(in.signal.text));
  }

 private:
  TsiId id_{};
  std::string instance_name_;
};

} // namespace tsiemene
