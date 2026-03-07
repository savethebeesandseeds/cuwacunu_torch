// ./include/tsiemene/tsi.sink.log.sys.h
// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

#include "camahjucunu/data/observation_sample.h"
#include "piaabo/dlogs.h"
#include "tsiemene/tsi.sink.h"

namespace tsiemene {

class TsiSinkLogSys final : public TsiSink {
 public:
  enum class LogMode : std::uint8_t {
    EachEvent = 0,
    EachBatch = 1,
    EachEpoch = 2,
  };

  [[nodiscard]] static std::optional<LogMode> parse_mode_token(
      std::string_view token) {
    if (token == "event" || token == "each_event") return LogMode::EachEvent;
    if (token == "batch" || token == "each_batch") return LogMode::EachBatch;
    if (token == "epoch" || token == "each_epoch") return LogMode::EachEpoch;
    return std::nullopt;
  }

  static constexpr DirectiveId IN_INFO  = directive_id::Info;
  static constexpr DirectiveId IN_WARN  = directive_id::Warn;
  static constexpr DirectiveId IN_DEBUG = directive_id::Debug;
  static constexpr DirectiveId IN_ERROR = directive_id::Error;

  explicit TsiSinkLogSys(TsiId id,
                         std::string instance_name = "tsi.probe.log",
                         LogMode mode = LogMode::EachEvent)
      : id_(id),
        instance_name_(std::move(instance_name)),
        mode_(mode) {}

  [[nodiscard]] std::string_view type_name() const noexcept override { return "tsi.probe.log"; }
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

  [[nodiscard]] bool is_compatible(DirectiveId target_incoming_directive,
                                   PayloadKind source_outgoing_kind) const noexcept override {
    if (source_outgoing_kind == PayloadKind::Cargo &&
        (target_incoming_directive == IN_INFO ||
         target_incoming_directive == IN_WARN ||
         target_incoming_directive == IN_DEBUG ||
         target_incoming_directive == IN_ERROR)) {
      return true;
    }
    return Tsi::is_compatible(target_incoming_directive, source_outgoing_kind);
  }

  void step(const Wave& wave, Ingress in, BoardContext& ctx, Emitter&) override {
    if (!ctx.debug_enabled) return;
    // Sink logs are high-volume trace output; print them but avoid buffering.
    const cuwacunu::piaabo::dlog_buffer_capture_scope no_buffer_capture(false);
    if (!should_emit_for_wave_(wave)) return;
    if (in.signal.kind == PayloadKind::Cargo) {
      if (in.directive == IN_DEBUG) {
        log_info("[tsi.probe.log.debug] wave(id=%llu,episode=%llu,batch=%llu,i=%llu) %s\n",
                 (unsigned long long)wave.cursor.id,
                 (unsigned long long)wave.cursor.episode,
                 (unsigned long long)wave.cursor.batch,
                 (unsigned long long)wave.cursor.i,
                 summarize_cargo_(in.signal.cargo).c_str());
        return;
      }
      if (in.directive == IN_WARN) {
        log_warn("[tsi.probe.log.warn] wave(id=%llu,episode=%llu,batch=%llu,i=%llu) %s\n",
                 (unsigned long long)wave.cursor.id,
                 (unsigned long long)wave.cursor.episode,
                 (unsigned long long)wave.cursor.batch,
                 (unsigned long long)wave.cursor.i,
                 summarize_cargo_(in.signal.cargo).c_str());
        return;
      }
      if (in.directive == IN_ERROR) {
        log_err("[tsi.probe.log.error] wave(id=%llu,episode=%llu,batch=%llu,i=%llu) %s\n",
                (unsigned long long)wave.cursor.id,
                (unsigned long long)wave.cursor.episode,
                (unsigned long long)wave.cursor.batch,
                (unsigned long long)wave.cursor.i,
                summarize_cargo_(in.signal.cargo).c_str());
        return;
      }
      if (in.directive == IN_INFO) {
        log_info("[tsi.probe.log.info] wave(id=%llu,episode=%llu,batch=%llu,i=%llu) %s\n",
                 (unsigned long long)wave.cursor.id,
                 (unsigned long long)wave.cursor.episode,
                 (unsigned long long)wave.cursor.batch,
                 (unsigned long long)wave.cursor.i,
                 summarize_cargo_(in.signal.cargo).c_str());
        return;
      }
    }

    if (in.directive == IN_DEBUG && in.signal.kind == PayloadKind::String) {
      log_info("[tsi.probe.log.debug] wave(id=%llu,episode=%llu,batch=%llu,i=%llu) %s\n",
               (unsigned long long)wave.cursor.id,
               (unsigned long long)wave.cursor.episode,
               (unsigned long long)wave.cursor.batch,
               (unsigned long long)wave.cursor.i,
               in.signal.text.c_str());
      return;
    }

    if (in.directive == IN_WARN && in.signal.kind == PayloadKind::String) {
      log_warn("[tsi.probe.log.warn] wave(id=%llu,episode=%llu,batch=%llu,i=%llu) %s\n",
               (unsigned long long)wave.cursor.id,
               (unsigned long long)wave.cursor.episode,
               (unsigned long long)wave.cursor.batch,
               (unsigned long long)wave.cursor.i,
               in.signal.text.c_str());
      return;
    }

    if (in.directive == IN_ERROR && in.signal.kind == PayloadKind::String) {
      log_err("[tsi.probe.log.error] wave(id=%llu,episode=%llu,batch=%llu,i=%llu) %s\n",
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
        log_info("[tsi.probe.log.info] wave(id=%llu,episode=%llu,batch=%llu,i=%llu) tensor0=%f\n",
                 (unsigned long long)wave.cursor.id,
                 (unsigned long long)wave.cursor.episode,
                 (unsigned long long)wave.cursor.batch,
                 (unsigned long long)wave.cursor.i,
                 (double)v);
      } else {
        log_warn("[tsi.probe.log.info] wave(id=%llu,episode=%llu,batch=%llu,i=%llu) tensor=<undefined>\n",
                 (unsigned long long)wave.cursor.id,
                 (unsigned long long)wave.cursor.episode,
                 (unsigned long long)wave.cursor.batch,
                 (unsigned long long)wave.cursor.i);
      }
      return;
    }
  }

 private:
  [[nodiscard]] bool should_emit_for_wave_(const Wave& wave) {
    if (mode_ == LogMode::EachEvent) return true;
    if (mode_ == LogMode::EachEpoch) {
      if (has_last_emit_scope_ && last_emit_episode_ == wave.cursor.episode) {
        return false;
      }
      has_last_emit_scope_ = true;
      last_emit_episode_ = wave.cursor.episode;
      last_emit_batch_ = wave.cursor.batch;
      return true;
    }
    if (has_last_emit_scope_ &&
        last_emit_episode_ == wave.cursor.episode &&
        last_emit_batch_ == wave.cursor.batch) {
      return false;
    }
    has_last_emit_scope_ = true;
    last_emit_episode_ = wave.cursor.episode;
    last_emit_batch_ = wave.cursor.batch;
    return true;
  }

  static std::string tensor_shape_(const torch::Tensor& t) {
    if (!t.defined()) return "undefined";
    std::ostringstream oss;
    oss << "[";
    const auto sizes = t.sizes();
    for (std::size_t i = 0; i < sizes.size(); ++i) {
      if (i > 0) oss << "x";
      oss << sizes[i];
    }
    oss << "]";
    return oss.str();
  }

  static std::string summarize_cargo_(const ObservationCargoPtr& cargo) {
    static constexpr const char* kReset = "\033[0m";
    static constexpr const char* kLabel = "\033[1;36m";
    static constexpr const char* kValue = "\033[38;5;220m";
    static constexpr const char* kDim = "\033[38;5;121m";

    std::ostringstream oss;
    if (!cargo) {
      oss << kLabel << "cargo" << kReset << "{"
          << kValue << "null" << kReset << "}";
      return oss.str();
    }

    const auto& c = *cargo;
    oss << kLabel << "cargo" << kReset << "{"
        << kLabel << "ptr" << kReset << "=" << kValue
        << static_cast<const void*>(cargo.get()) << kReset
        << ", " << kLabel << "features" << kReset << "=" << kDim << tensor_shape_(c.features) << kReset
        << ", " << kLabel << "mask" << kReset << "=" << kDim << tensor_shape_(c.mask) << kReset
        << ", " << kLabel << "future_features" << kReset << "=" << kDim << tensor_shape_(c.future_features) << kReset
        << ", " << kLabel << "future_mask" << kReset << "=" << kDim << tensor_shape_(c.future_mask) << kReset
        << ", " << kLabel << "encoding" << kReset << "=" << kDim << tensor_shape_(c.encoding) << kReset
        << ", " << kLabel << "past_keys" << kReset << "=" << kDim << tensor_shape_(c.past_keys) << kReset
        << ", " << kLabel << "future_keys" << kReset << "=" << kDim << tensor_shape_(c.future_keys) << kReset
        << ", " << kLabel << "mean" << kReset << "=" << kDim << tensor_shape_(c.feature_mean) << kReset
        << ", " << kLabel << "std" << kReset << "=" << kDim << tensor_shape_(c.feature_std) << kReset
        << ", " << kLabel << "normalized" << kReset << "=" << kValue << (c.normalized ? "true" : "false") << kReset
        << "}";
    return oss.str();
  }

  TsiId id_{};
  std::string instance_name_;
  LogMode mode_{LogMode::EachEvent};
  bool has_last_emit_scope_{false};
  std::uint64_t last_emit_episode_{0};
  std::uint64_t last_emit_batch_{0};
};

} // namespace tsiemene
