// ./include/tsiemene/tsi.wikimyei.representation.vicreg.h
// SPDX-License-Identifier: MIT
#pragma once

#include <cstddef>
#include <sstream>
#include <string>
#include <string_view>

#include <torch/torch.h>

#include "tsiemene/utils/tsi.h"

// Your existing VICReg implementation:
#include "wikimyei/representation/VICReg/vicreg_4d.h"

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
  static constexpr DirectiveId IN_PAYLOAD  = directive_id::Payload;
  static constexpr DirectiveId OUT_PAYLOAD = directive_id::Payload;
  static constexpr DirectiveId OUT_LOSS    = directive_id::Loss;
  static constexpr DirectiveId OUT_META    = directive_id::Meta;

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

  [[nodiscard]] std::string_view type_name() const noexcept override { return "tsi.wikimyei.representation.vicreg"; }
  [[nodiscard]] std::string_view instance_name() const noexcept override { return instance_name_; }
  [[nodiscard]] TsiId id() const noexcept override { return id_; }

  [[nodiscard]] std::span<const DirectiveSpec> directives() const noexcept override {
    static constexpr DirectiveSpec kDirectives[] = {
      directive(IN_PAYLOAD, DirectiveDir::In, KindSpec::Tensor(), "packed [B,C,T,D+1] (last=D is mask)"),
      directive(OUT_PAYLOAD, DirectiveDir::Out, KindSpec::Tensor(), "representation encoding"),
      directive(OUT_LOSS, DirectiveDir::Out, KindSpec::Tensor(), "loss scalar (only when train=true)"),
      directive(OUT_META, DirectiveDir::Out, KindSpec::String(), "runtime trace/meta stream"),
    };
    return std::span<const DirectiveSpec>(kDirectives, 4);
  }

  void set_train(bool on) noexcept { train_ = on; }

  void step(const Wave& wave, Ingress in, TsiContext&, Emitter& out) override {
    if (in.directive != IN_PAYLOAD) return;
    if (in.signal.kind != PayloadKind::Tensor) return;

    {
      std::ostringstream oss;
      oss << "vicreg.in packed=" << tensor_shape_(in.signal.tensor)
          << " train=" << (train_ ? "true" : "false")
          << " detach_to_cpu=" << (detach_to_cpu_ ? "true" : "false");
      emit_meta_(wave, out, oss.str());
    }

    torch::Tensor data, mask;
    unpack_vicreg_packed_batch(in.signal.tensor, model_.D, data, mask);

    data = data.to(model_.device);
    mask = mask.to(model_.device);

    // Always emit representation.
    auto repr = model_.encode(data, mask,
                              /*use_swa=*/use_swa_,
                              /*detach_to_cpu=*/detach_to_cpu_);
    if (detach_to_cpu_) repr = repr.cpu();
    const std::string repr_shape = tensor_shape_(repr);
    out.emit_tensor(wave, OUT_PAYLOAD, std::move(repr));
    emit_meta_(wave, out, std::string("vicreg.out payload=") + repr_shape);

    // Training mode: you will drop your old fit-step logic here.
    // For now (so the plumbing tests), emit a placeholder loss.
    if (train_) {
      auto loss = torch::zeros({}, torch::TensorOptions().device(torch::kCPU).dtype(torch::kFloat32));
      const std::string loss_shape = tensor_shape_(loss);
      out.emit_tensor(wave, OUT_LOSS, std::move(loss));
      emit_meta_(wave, out, std::string("vicreg.out loss=") + loss_shape);
    }
  }

 private:
  static std::string tensor_shape_(const torch::Tensor& t) {
    if (!t.defined()) return ":tensor undefined";
    std::ostringstream oss;
    oss << ":tensor shape=[";
    const auto sizes = t.sizes();
    for (std::size_t i = 0; i < sizes.size(); ++i) {
      if (i > 0) oss << ",";
      oss << sizes[i];
    }
    oss << "]";
    return oss.str();
  }

  void emit_meta_(const Wave& wave, Emitter& out, std::string msg) const {
    out.emit_string(wave, OUT_META, std::move(msg));
  }

  TsiId id_{};
  std::string instance_name_;

  bool train_{false};
  bool use_swa_{true};
  bool detach_to_cpu_{true};

  cuwacunu::wikimyei::vicreg_4d::VICReg_4D model_;
};

} // namespace tsiemene
