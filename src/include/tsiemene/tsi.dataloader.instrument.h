// ./include/tsiemene/tsi.dataloader.instrument.h
// SPDX-License-Identifier: MIT
#pragma once

#include <charconv>
#include <cstdint>
#include <string>
#include <string_view>

#include <torch/torch.h>

#include "tsiemene/tsi.h"

namespace tsiemene {

class TsiDataloaderInstrument final : public Tsi {
 public:
  static constexpr PortId IN_CMD    = 1;
  static constexpr PortId OUT_BATCH = 2;

  static constexpr std::string_view TAG_CMD   = "dataloader.cmd";
  static constexpr std::string_view TAG_BATCH = "vicreg4d.packed_batch";

  TsiDataloaderInstrument(TsiId id,
                          std::string instrument,
                          int B, int C, int T, int D,
                          torch::Device device = torch::kCPU)
      : id_(id),
        instrument_(std::move(instrument)),
        type_name_(std::string("tsi.dataloader.") + instrument_),
        instance_name_(type_name_),
        B_(B), C_(C), T_(T), D_(D),
        device_(device) {}

  [[nodiscard]] std::string_view type_name() const noexcept override { return type_name_; }
  [[nodiscard]] std::string_view instance_name() const noexcept override { return instance_name_; }
  [[nodiscard]] TsiId id() const noexcept override { return id_; }

  [[nodiscard]] std::span<const Port> ports() const noexcept override {
    static constexpr Port kPorts[] = {
      port(IN_CMD,    PortDir::In,  Schema::String(), TAG_CMD,   "command for this wave (e.g. \"batches=10\")"),
      port(OUT_BATCH, PortDir::Out, Schema::Tensor(), TAG_BATCH, "packed [B,C,T,D+1] (last=D is mask 0/1)"),
    };
    return std::span<const Port>(kPorts, 2);
  }

  [[nodiscard]] Determinism determinism() const noexcept override {
    // Synthetic random batches are not deterministic unless you seed torch deterministically.
    return Determinism::SeededStochastic;
  }

  void step(const Wave& wave, Ingress in, TsiContext&, Emitter& out) override {
    if (in.port != IN_CMD) return;
    if (in.signal.kind != PayloadKind::String) return;

    const std::uint64_t n = parse_batches_(in.signal.text);
    for (std::uint64_t k = 0; k < n; ++k) {
      Wave witem{.id = wave.id, .i = wave.i + k};
      out.emit_tensor(witem, OUT_BATCH, make_packed_batch_());
    }
  }

 private:
  static std::uint64_t parse_batches_(std::string_view s) noexcept {
    // Accept: "batches=10" or "10" (we just parse the first digit run).
    std::size_t pos = 0;
    while (pos < s.size() && (s[pos] < '0' || s[pos] > '9')) ++pos;
    if (pos == s.size()) return 0;

    std::uint64_t v = 0;
    auto sv = s.substr(pos);
    auto res = std::from_chars(sv.data(), sv.data() + sv.size(), v);
    if (res.ec != std::errc()) return 0;
    return v;
  }

  torch::Tensor make_packed_batch_() const {
    // data: [B,C,T,D]
    auto data = torch::rand({B_, C_, T_, D_},
                            torch::TensorOptions().device(device_).dtype(torch::kFloat32));
    // mask: [B,C,T] (all valid)
    auto mask = torch::ones({B_, C_, T_},
                            torch::TensorOptions().device(device_).dtype(torch::kFloat32));
    // pack: [B,C,T,D+1]
    return torch::cat({data, mask.unsqueeze(-1)}, /*dim=*/3);
  }

 private:
  TsiId id_{};
  std::string instrument_;
  std::string type_name_;
  std::string instance_name_;

  int B_{}, C_{}, T_{}, D_{};
  torch::Device device_{torch::kCPU};
};

} // namespace tsiemene
