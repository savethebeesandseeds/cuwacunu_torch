// ./include/tsiemene/tsi.sink.log.sys.h
// SPDX-License-Identifier: MIT
#pragma once

#include <string>
#include <string_view>

#include "piaabo/dlogs.h"
#include "tsiemene/tsi.sink.h"

namespace tsiemene {

class TsiSinkLogSys final : public TsiSink {
 public:
  static constexpr DirectiveId IN_INFO  = directive_id::Info;
  static constexpr DirectiveId IN_WARN  = directive_id::Warn;
  static constexpr DirectiveId IN_DEBUG = directive_id::Debug;
  static constexpr DirectiveId IN_ERROR = directive_id::Error;

  explicit TsiSinkLogSys(TsiId id,
                         std::string instance_name = "tsi.sink.log.sys")
      : id_(id),
        instance_name_(std::move(instance_name)) {}

  [[nodiscard]] std::string_view type_name() const noexcept override { return "tsi.sink.log.sys"; }
  [[nodiscard]] std::string_view instance_name() const noexcept override { return instance_name_; }
  [[nodiscard]] TsiId id() const noexcept override { return id_; }
  [[nodiscard]] bool suppress_runtime_meta_feedback() const noexcept override { return true; }

  [[nodiscard]] std::span<const DirectiveSpec> directives() const noexcept override {
    static constexpr DirectiveSpec kDirectives[] = {
      directive(IN_INFO, DirectiveDir::In, KindSpec::Tensor(), "log info tensor payloads"),
      directive(IN_WARN, DirectiveDir::In, KindSpec::String(), "warning messages"),
      directive(IN_DEBUG, DirectiveDir::In, KindSpec::String(), "debug/meta messages"),
      directive(IN_ERROR, DirectiveDir::In, KindSpec::String(), "error messages"),
    };
    return std::span<const DirectiveSpec>(kDirectives, 4);
  }

  void step(const Wave& wave, Ingress in, BoardContext&, Emitter&) override {
    if (in.directive == IN_DEBUG && in.signal.kind == PayloadKind::String) {
      log_info("[tsi.log.sys.debug] wave(id=%llu,episode=%llu,batch=%llu,i=%llu) %s\n",
               (unsigned long long)wave.cursor.id,
               (unsigned long long)wave.cursor.episode,
               (unsigned long long)wave.cursor.batch,
               (unsigned long long)wave.cursor.i,
               in.signal.text.c_str());
      return;
    }

    if (in.directive == IN_WARN && in.signal.kind == PayloadKind::String) {
      log_warn("[tsi.log.sys.warn] wave(id=%llu,episode=%llu,batch=%llu,i=%llu) %s\n",
               (unsigned long long)wave.cursor.id,
               (unsigned long long)wave.cursor.episode,
               (unsigned long long)wave.cursor.batch,
               (unsigned long long)wave.cursor.i,
               in.signal.text.c_str());
      return;
    }

    if (in.directive == IN_ERROR && in.signal.kind == PayloadKind::String) {
      log_err("[tsi.log.sys.error] wave(id=%llu,episode=%llu,batch=%llu,i=%llu) %s\n",
              (unsigned long long)wave.cursor.id,
              (unsigned long long)wave.cursor.episode,
              (unsigned long long)wave.cursor.batch,
              (unsigned long long)wave.cursor.i,
              in.signal.text.c_str());
      return;
    }

    if (in.directive == IN_INFO && in.signal.kind == PayloadKind::Tensor) {
      const auto& t = in.signal.tensor;
      if (t.defined() && t.numel() > 0) {
        auto info_cpu = t.detach().to(torch::kCPU).reshape({-1});
        const float v = info_cpu[0].item<float>();
        log_info("[tsi.log.sys.info] wave(id=%llu,episode=%llu,batch=%llu,i=%llu) tensor0=%f\n",
                 (unsigned long long)wave.cursor.id,
                 (unsigned long long)wave.cursor.episode,
                 (unsigned long long)wave.cursor.batch,
                 (unsigned long long)wave.cursor.i,
                 (double)v);
      } else {
        log_warn("[tsi.log.sys.info] wave(id=%llu,episode=%llu,batch=%llu,i=%llu) tensor=<undefined>\n",
                 (unsigned long long)wave.cursor.id,
                 (unsigned long long)wave.cursor.episode,
                 (unsigned long long)wave.cursor.batch,
                 (unsigned long long)wave.cursor.i);
      }
      return;
    }
  }

 private:
  TsiId id_{};
  std::string instance_name_;
};

} // namespace tsiemene
