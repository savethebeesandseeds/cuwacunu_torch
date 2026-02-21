// ./include/tsiemene/tsi.sink.null.h
// SPDX-License-Identifier: MIT
#pragma once

#include <string>
#include <string_view>

#include "tsiemene/utils/tsi.h"

namespace tsiemene {

class TsiSinkNull final : public TsiSink {
 public:
  static constexpr DirectiveId IN_PAYLOAD = directive_id::Payload;
  static constexpr DirectiveId OUT_META   = directive_id::Meta;

  explicit TsiSinkNull(TsiId id,
                       std::string instance_name = "tsi.sink.null")
      : id_(id),
        instance_name_(std::move(instance_name)) {}

  [[nodiscard]] std::string_view type_name() const noexcept override { return "tsi.sink.null"; }
  [[nodiscard]] std::string_view instance_name() const noexcept override { return instance_name_; }
  [[nodiscard]] TsiId id() const noexcept override { return id_; }

  [[nodiscard]] std::span<const DirectiveSpec> directives() const noexcept override {
    static constexpr DirectiveSpec kDirectives[] = {
      directive(IN_PAYLOAD, DirectiveDir::In, KindSpec::Tensor(), "drop tensor payload"),
      directive(OUT_META, DirectiveDir::Out, KindSpec::String(), "runtime trace/meta stream"),
    };
    return std::span<const DirectiveSpec>(kDirectives, 2);
  }

  void step(const Wave&, Ingress in, TsiContext&, Emitter&) override {
    if (in.directive != IN_PAYLOAD) return;
    if (in.signal.kind != PayloadKind::Tensor) return;
    // Intentionally no-op: consume and discard.
  }

 private:
  TsiId id_{};
  std::string instance_name_;
};

} // namespace tsiemene
