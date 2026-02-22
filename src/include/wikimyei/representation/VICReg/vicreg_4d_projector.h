/* vicreg_4d_projector.h */
#pragma once

#include <torch/torch.h>
#include <vector>
#include <string>
#include <sstream>

namespace cuwacunu {
namespace wikimyei {
namespace vicreg_4d {

enum class NormKind { BatchNorm1d, LayerNorm, None };
enum class ActKind  { ReLU, SiLU };

/** Explicit options (no defaults in this struct to avoid unspecified behavior). */
struct ProjectorOptions {
  NormKind norm_kind;         // BatchNorm1d, LayerNorm, or None
  ActKind  act_kind;          // ReLU or SiLU
  bool     use_hidden_bias;   // true: keep bias in hidden Linear; false: disable (recommended when using BN)
  bool     use_last_bias;     // true: allow bias in last Linear; often false in VICReg heads
  bool     bn_in_fp32;        // true: force BN compute/params in FP32 (recommended if dtype != kFloat32)
};

/** BN wrapper that always computes in FP32 and casts back to input dtype. */
class BN1dFP32Impl : public torch::nn::Module {
public:
  explicit BN1dFP32Impl(int64_t features)
  : bn(torch::nn::BatchNorm1d(torch::nn::BatchNorm1dOptions(features))) {
    register_module("bn", bn);
    // Keep BN parameters/stats in FP32.
    bn->to(torch::kFloat32);
  }

  torch::Tensor forward(torch::Tensor x) {
    const auto in_dtype  = x.scalar_type();
    // Flattened input is [N, F]; BN1d expects exactly that.
    auto y = bn->forward(x.to(torch::kFloat32));
    return y.to(in_dtype);
  }

private:
  torch::nn::BatchNorm1d bn{nullptr};
};
TORCH_MODULE(BN1dFP32);

class VICReg_4D_ProjectorImpl : public torch::nn::Module {
public:
  /** Compatibility ctor (kept for source compatibility). Behavior matches your current code. */
  VICReg_4D_ProjectorImpl(
    int embedding_dim,
    const std::string& mlp_spec,
    torch::Dtype dtype_ = torch::kFloat32,
    torch::Device device_ = torch::kCPU
  ) : embedding_dim(embedding_dim),
      mlp_spec(mlp_spec),
      dtype(dtype_), device(device_),
      explicit_opts(false) // compatibility mode
  {
    // Default compatibility behavior: BN + ReLU, biases in hidden, last bias off
    opts_compat.norm_kind       = NormKind::BatchNorm1d;
    opts_compat.act_kind        = ActKind::ReLU;
    opts_compat.use_hidden_bias = true;      // match your original
    opts_compat.use_last_bias   = false;     // match your original
    opts_compat.bn_in_fp32      = (dtype != torch::kFloat32);
    reset();
  }

  /** New ctor with **explicit** options (no defaults). */
  VICReg_4D_ProjectorImpl(
    int embedding_dim,
    const std::string& mlp_spec,
    const ProjectorOptions& options,
    torch::Dtype dtype_,
    torch::Device device_
  ) : embedding_dim(embedding_dim),
      mlp_spec(mlp_spec),
      dtype(dtype_), device(device_),
      explicit_opts(true), opts_explicit(options)
  {
    reset();
  }

  void reset() {
    // Remove old seq
    if (layers) { unregister_module("layers"); }
    layers = register_module("layers", torch::nn::Sequential());

    // Choose options source
    const ProjectorOptions& O = explicit_opts ? opts_explicit : opts_compat;

    // Parse dims (input embed → hidden… → output)
    auto dims = parse_mlp_spec(embedding_dim, mlp_spec);
    TORCH_CHECK(dims.size() >= 2, "[VICReg_4D_Projector] MLP spec must contain at least one output dimension");

    for (size_t i = 0; i + 1 < dims.size(); ++i) {
      const bool last = (i + 2 == dims.size());
      const int inF   = dims[i];
      const int outF  = dims[i + 1];

      // 1) Linear
      const bool use_bias = last ? O.use_last_bias
                                 : (O.norm_kind == NormKind::BatchNorm1d ? false : O.use_hidden_bias);
      auto lin = torch::nn::Linear(torch::nn::LinearOptions(inF, outF).bias(use_bias));
      layers->push_back(lin);

      // Initialization
      if (!last) {
        // Kaiming for hidden layers (match activation)
        const auto nonlin = (O.act_kind == ActKind::ReLU) ? torch::kReLU : torch::kReLU; // same fan-in setting
        torch::nn::init::kaiming_normal_(lin->weight, /*a=*/0.0, torch::kFanIn, nonlin);
        if (use_bias) torch::nn::init::zeros_(lin->bias);
      } else {
        // Last layer: Xavier is common for heads
        torch::nn::init::xavier_uniform_(lin->weight);
        if (use_bias) torch::nn::init::zeros_(lin->bias);
      }

      if (!last) {
        // 2) Normalization
        switch (O.norm_kind) {
          case NormKind::BatchNorm1d: {
            if (O.bn_in_fp32 && dtype != torch::kFloat32) {
              layers->push_back(BN1dFP32(outF));
            } else {
              auto bn = torch::nn::BatchNorm1d(torch::nn::BatchNorm1dOptions(outF));
              layers->push_back(bn);
            }
          } break;
          case NormKind::LayerNorm: {
            // LayerNorm over feature dim (works well w/ masks; no batch dependence)
            auto ln = torch::nn::LayerNorm(torch::nn::LayerNormOptions({outF}));
            layers->push_back(ln);
          } break;
          case NormKind::None:
            // no norm
            break;
        }

        // 3) Activation
        if (O.act_kind == ActKind::ReLU) {
          layers->push_back(torch::nn::ReLU(torch::nn::ReLUOptions().inplace(true)));
        } else { // SiLU
          layers->push_back(torch::nn::SiLU());
        }
      }
    }

    // Move the whole stack to the desired device/dtype.
    // NOTE: BN1dFP32 modules keep their params in FP32 regardless of this cast.
    layers->to(device, dtype);
    this->to(device, dtype);
  }

  torch::Tensor forward_flat(torch::Tensor x2d) {
    TORCH_CHECK(x2d.dim() == 2, "[Projector] forward_flat expects [N, E]");
    return layers->forward(x2d);
  }

  /** Forward: x is [B, T, E]. */
  torch::Tensor forward(torch::Tensor x) {
    TORCH_CHECK(x.dim() == 3, "[VICReg_4D_Projector] expected x as [B, T, E]");
    const auto B = x.size(0);
    const auto T = x.size(1);

    auto xt = x.reshape({B * T, x.size(-1)}); // [B·T, E]
    xt = layers->forward(xt);                 // [B·T, E’]
    xt = xt.view({B, T, xt.size(-1)});        // [B, T, E’]
    return xt;
  }

private:
  int                 embedding_dim;
  std::string         mlp_spec;
  torch::Dtype        dtype;
  torch::Device       device;

  bool                explicit_opts;
  ProjectorOptions    opts_explicit; // used when explicit_opts == true
  ProjectorOptions    opts_compat;   // populated in compatibility ctor

  torch::nn::Sequential layers{nullptr};

  static std::vector<int> parse_mlp_spec(int embed, const std::string& spec) {
    std::vector<int> out; out.reserve(8);
    out.push_back(embed);
    std::stringstream ss(spec);
    std::string tok;
    while (std::getline(ss, tok, '-')) {
      // Trim whitespace
      size_t l = 0, r = tok.size();
      while (l < r && std::isspace(static_cast<unsigned char>(tok[l]))) ++l;
      while (r > l && std::isspace(static_cast<unsigned char>(tok[r-1]))) --r;
      TORCH_CHECK(l < r, "[VICReg_4D_Projector] empty token in mlp_spec: '", spec, "'");
      int v = std::stoi(tok.substr(l, r - l));
      TORCH_CHECK(v > 0, "[VICReg_4D_Projector] non-positive layer width in mlp_spec: ", v);
      out.push_back(v);
    }
    return out;
  }
};

TORCH_MODULE(VICReg_4D_Projector);

} /* namespace vicreg_4d */
} /* namespace wikimyei */
} /* namespace cuwacunu */
