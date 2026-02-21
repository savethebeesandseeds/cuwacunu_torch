// ./include/tsiemene/tsi.sink.log.sys.h
// SPDX-License-Identifier: MIT
#pragma once

#include <string>
#include <string_view>

#include "piaabo/dlogs.h"
#include "tsiemene/utils/tsi.h"

namespace tsiemene {

class TsiSinkLogSys final : public TsiSink {
 public:
  static constexpr DirectiveId IN_PAYLOAD = directive_id::Payload;
  static constexpr DirectiveId IN_LOSS    = directive_id::Loss;
  static constexpr DirectiveId IN_META    = directive_id::Meta;

  explicit TsiSinkLogSys(TsiId id,
                         std::string instance_name = "tsi.sink.log.sys")
      : id_(id),
        instance_name_(std::move(instance_name)) {}

  [[nodiscard]] std::string_view type_name() const noexcept override { return "tsi.sink.log.sys"; }
  [[nodiscard]] std::string_view instance_name() const noexcept override { return instance_name_; }
  [[nodiscard]] TsiId id() const noexcept override { return id_; }

  [[nodiscard]] std::span<const DirectiveSpec> directives() const noexcept override {
    static constexpr DirectiveSpec kDirectives[] = {
      directive(IN_PAYLOAD, DirectiveDir::In, KindSpec::String(), "log string payload"),
      directive(IN_LOSS, DirectiveDir::In, KindSpec::Tensor(), "log loss tensor"),
      directive(IN_META, DirectiveDir::In, KindSpec::String(), "runtime/system trace messages"),
    };
    return std::span<const DirectiveSpec>(kDirectives, 3);
  }

  void step(const Wave& wave, Ingress in, TsiContext&, Emitter&) override {
    if (in.directive == IN_META && in.signal.kind == PayloadKind::String) {
      log_info("[tsi.log.sys.meta] wave(id=%llu,i=%llu) %s\n",
               (unsigned long long)wave.id,
               (unsigned long long)wave.i,
               in.signal.text.c_str());
      return;
    }

    if (in.directive == IN_PAYLOAD && in.signal.kind == PayloadKind::String) {
      log_info("[tsi.log.sys] wave(id=%llu,i=%llu) %s\n",
               (unsigned long long)wave.id,
               (unsigned long long)wave.i,
               in.signal.text.c_str());
      return;
    }

    if (in.directive == IN_LOSS && in.signal.kind == PayloadKind::Tensor) {
      const auto& t = in.signal.tensor;
      if (t.defined() && t.numel() > 0) {
        auto loss_cpu = t.detach().to(torch::kCPU).reshape({-1});
        const float v = loss_cpu[0].item<float>();
        log_info("[tsi.log.sys] wave(id=%llu,i=%llu) loss=%f\n",
                 (unsigned long long)wave.id,
                 (unsigned long long)wave.i,
                 (double)v);
      } else {
        log_warn("[tsi.log.sys] wave(id=%llu,i=%llu) loss=<undefined>\n",
                 (unsigned long long)wave.id,
                 (unsigned long long)wave.i);
      }
      return;
    }
  }

 private:
  TsiId id_{};
  std::string instance_name_;
};

} // namespace tsiemene
