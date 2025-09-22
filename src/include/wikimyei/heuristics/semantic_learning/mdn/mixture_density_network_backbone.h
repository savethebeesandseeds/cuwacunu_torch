/* mixture_density_network_backbone.h */
#pragma once
#include <torch/torch.h>
#include <vector>

#include "camahjucunu/BNF/implementations/training_components/training_components.h"
#include "wikimyei/heuristics/semantic_learning/mdn/mixture_density_network_types.h"

namespace cuwacunu {
namespace wikimyei {
namespace mdn {

// =============================
// Residual MLP Block
// =============================
struct ResMLPBlockImpl : torch::nn::Module {
  torch::nn::Linear    fc1{nullptr}, fc2{nullptr};
  torch::nn::LayerNorm ln1{nullptr}, ln2{nullptr};

  explicit ResMLPBlockImpl(const residualOptions& opt)
  : fc1(register_module("fc1", torch::nn::Linear(opt.in_dim,    opt.hidden))),
    fc2(register_module("fc2", torch::nn::Linear(opt.hidden,    opt.in_dim))),
    ln1(register_module("ln1", torch::nn::LayerNorm(torch::nn::LayerNormOptions(std::vector<int64_t>{opt.in_dim})))),
    ln2(register_module("ln2", torch::nn::LayerNorm(torch::nn::LayerNormOptions(std::vector<int64_t>{opt.in_dim})))) {}

  torch::Tensor forward(const torch::Tensor& x) {
    auto h = ln1->forward(x);
    h = torch::silu(fc1->forward(h));
    h = fc2->forward(h);
    auto y = x + h;
    return torch::silu(ln2->forward(y));
  }
};
TORCH_MODULE(ResMLPBlock);

// =============================
// Backbone: simple MLP with residuals
// =============================
struct BackboneImpl : torch::nn::Module {
  torch::nn::Linear in{nullptr};
  std::vector<ResMLPBlock> blocks;
  torch::nn::LayerNorm out_norm{nullptr};

  explicit BackboneImpl(const BackboneOptions& opt) {
    in = register_module("in", torch::nn::Linear(opt.input_dim, opt.feature_dim));
    blocks.reserve(opt.depth);
    for (int i = 0; i < opt.depth; ++i) {
      residualOptions ro{opt.feature_dim, opt.feature_dim * 2};
      auto blk = ResMLPBlock(ro);
      blocks.push_back(register_module("blk" + std::to_string(i), blk));
    }
    out_norm = register_module("out_norm",
      torch::nn::LayerNorm(torch::nn::LayerNormOptions(std::vector<int64_t>{opt.feature_dim})));
  }

  torch::Tensor forward(const torch::Tensor& x) {
    auto h = torch::silu(in->forward(x));
    for (auto& blk : blocks) h = blk->forward(h);
    return out_norm->forward(h);
  }
};
TORCH_MODULE(Backbone);

} // namespace mdn
} // namespace wikimyei
} // namespace cuwacunu
