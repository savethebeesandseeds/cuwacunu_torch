// ./include/tsiemene/tsi.representation.vicreg.h
// SPDX-License-Identifier: MIT
#pragma once

#include <string>
#include <string_view>

#include <torch/torch.h>

#include "tsiemene/tsi.h"

// Your existing VICReg implementation:
#include "wikimyei/heuristics/representation_learning/VICReg/vicreg_4d.h"

namespace tsiemene {

// packed: [B,C,T,D+1] where last slot is mask (0/1)
inline void unpack_vicreg_packed_batch(const torch::Tensor& packed,
                                       int D,
                                       torch::Tensor& data_out,
                                       torch::Tensor& mask_out) {
  TORCH_CHECK(packed.defined(), "[tsi.vicreg] packed batch undefined");
  TORCH_CHECK(packed.dim() == 4, "[tsi.vicreg] packed must be [B,C,T,D+1]");
  TORCH_CHECK(packed.size(3) == D + 1, "[tsi.vicreg] packed.size(3) must be D+1");

  data_out = packed.narrow(/*dim=*/3, /*start=*/0, /*length=*/D);
  mask_out = (packed.select(/*dim=*/3, /*index=*/D) > 0.5); // bool [B,C,T]
}

class TsiVicreg4D final : public Tsi {
 public:
  static constexpr PortId IN_BATCH = 1;
  static constexpr PortId OUT_REPR = 2;
  static constexpr PortId OUT_LOSS = 3;

  static constexpr std::string_view TAG_BATCH = "vicreg4d.packed_batch";
  static constexpr std::string_view TAG_REPR  = "vicreg4d.repr";
  static constexpr std::string_view TAG_LOSS  = "vicreg4d.loss";

  TsiVicreg4D(TsiId id,
              std::string instance_name,
              int C, int T, int D,
              bool train = false,
              bool use_swa = true,
              bool detach_to_cpu = true)
      : id_(id),
        instance_name_(std::move(instance_name)),
        train_(train),
        use_swa_(use_swa),
        detach_to_cpu_(detach_to_cpu),
        model_("VICReg_representation", C, T, D) {}

  [[nodiscard]] std::string_view type_name() const noexcept override { return "tsi.representation.vicreg4d"; }
  [[nodiscard]] std::string_view instance_name() const noexcept override { return instance_name_; }
  [[nodiscard]] TsiId id() const noexcept override { return id_; }

  [[nodiscard]] std::span<const Port> ports() const noexcept override {
    static constexpr Port kPorts[] = {
      port(IN_BATCH, PortDir::In,  Schema::Tensor(), TAG_BATCH, "packed [B,C,T,D+1] (last=D is mask)"),
      port(OUT_REPR, PortDir::Out, Schema::Tensor(), TAG_REPR,  "representation encoding"),
      port(OUT_LOSS, PortDir::Out, Schema::Tensor(), TAG_LOSS,  "loss scalar (only when train=true)"),
    };
    return std::span<const Port>(kPorts, 3);
  }

  void set_train(bool on) noexcept { train_ = on; }

  void step(const Wave& wave, Ingress in, TsiContext&, Emitter& out) override {
    if (in.port != IN_BATCH) return;
    if (in.signal.kind != PayloadKind::Tensor) return;

    torch::Tensor data, mask;
    unpack_vicreg_packed_batch(in.signal.tensor, model_.D, data, mask);

    data = data.to(model_.device);
    mask = mask.to(model_.device);

    // Always emit representation.
    auto repr = model_.encode(data, mask,
                              /*use_swa=*/use_swa_,
                              /*detach_to_cpu=*/detach_to_cpu_);
    if (detach_to_cpu_) repr = repr.cpu();
    out.emit_tensor(wave, OUT_REPR, std::move(repr));

    // Training mode: you will drop your old fit-step logic here.
    // For now (so the plumbing tests), emit a placeholder loss.
    if (train_) {
      auto loss = torch::zeros({}, torch::TensorOptions().device(torch::kCPU).dtype(torch::kFloat32));
      out.emit_tensor(wave, OUT_LOSS, std::move(loss));
    }
  }

 private:
  TsiId id_{};
  std::string instance_name_;

  bool train_{false};
  bool use_swa_{true};
  bool detach_to_cpu_{true};

  cuwacunu::wikimyei::vicreg_4d::VICReg_4D model_;
};

} // namespace tsiemene
