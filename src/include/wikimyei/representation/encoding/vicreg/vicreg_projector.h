// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <torch/torch.h>

namespace cuwacunu::wikimyei::representation::encoding::vicreg {

struct vicreg_projector_options_t {
  int64_t input_dim{0};
  int64_t projector_dim{0};
  int64_t hidden_dim{0};
  int64_t depth{1};
  torch::Dtype dtype{torch::kFloat32};
  torch::Device device{torch::kCPU};
};

namespace vicreg_projector_detail {

inline void validate_options(const vicreg_projector_options_t &opts) {
  if (opts.input_dim <= 0 || opts.projector_dim <= 0 || opts.depth < 0) {
    throw std::runtime_error("[vicreg_projector] invalid dimensions");
  }
  if (opts.depth > 0 && opts.hidden_dim <= 0) {
    throw std::runtime_error("[vicreg_projector] hidden_dim must be "
                             "positive when depth > 0");
  }
}

} // namespace vicreg_projector_detail

class VicregProjectorImpl : public torch::nn::Module {
public:
  explicit VicregProjectorImpl(vicreg_projector_options_t opts)
      : options_(std::move(opts)) {
    if (options_.hidden_dim <= 0) {
      options_.hidden_dim =
          std::max(options_.input_dim, options_.projector_dim);
    }
    vicreg_projector_detail::validate_options(options_);

    if (options_.depth == 0) {
      layers_.push_back(register_module(
          "projector_out",
          torch::nn::Linear(options_.input_dim, options_.projector_dim)));
    } else {
      layers_.push_back(register_module(
          "projector_in",
          torch::nn::Linear(options_.input_dim, options_.hidden_dim)));
      for (int64_t i = 1; i < options_.depth; ++i) {
        layers_.push_back(register_module(
            "projector_hidden_" + std::to_string(i),
            torch::nn::Linear(options_.hidden_dim, options_.hidden_dim)));
      }
      layers_.push_back(register_module(
          "projector_out",
          torch::nn::Linear(options_.hidden_dim, options_.projector_dim)));
    }
    this->to(options_.device, options_.dtype);
  }

  [[nodiscard]] const vicreg_projector_options_t &options() const {
    return options_;
  }

  [[nodiscard]] torch::Tensor forward(const torch::Tensor &z) {
    TORCH_CHECK(z.defined() && (z.dim() == 3 || z.dim() == 4),
                "[vicreg_projector] z must be [M,C,De] or "
                "[M,C,Hx,De]");
    TORCH_CHECK(z.size(z.dim() - 1) == options_.input_dim,
                "[vicreg_projector] input dim mismatch");
    auto x = z.to(
        torch::TensorOptions().dtype(options_.dtype).device(options_.device));
    const auto rows = x.numel() / options_.input_dim;
    x = x.reshape({rows, options_.input_dim});
    for (std::size_t i = 0; i < layers_.size(); ++i) {
      x = layers_[i]->forward(x);
      if (i + 1 < layers_.size()) {
        x = torch::gelu(x);
      }
    }
    if (z.dim() == 3) {
      return x.view({z.size(0), z.size(1), options_.projector_dim});
    }
    return x.view({z.size(0), z.size(1), z.size(2), options_.projector_dim});
  }

private:
  vicreg_projector_options_t options_{};
  std::vector<torch::nn::Linear> layers_{};
};

TORCH_MODULE(VicregProjector);

} // namespace cuwacunu::wikimyei::representation::encoding::vicreg
