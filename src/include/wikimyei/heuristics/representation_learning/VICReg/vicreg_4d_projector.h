/* vicreg_4d_projector.h */
#pragma once

#include <torch/torch.h>
#include <vector>
#include <string>
#include <sstream>

namespace cuwacunu {
namespace wikimyei {
namespace vicreg_4d {

class VICReg_4D_ProjectorImpl : public torch::nn::Module {
public:
  VICReg_4D_ProjectorImpl(
    int embedding_dim, 
    const std::string& mlp_spec,
    torch::Dtype dtype_ = torch::kFloat32,
    torch::Device device_ = torch::kCPU
  ) : embedding_dim(embedding_dim), 
      mlp_spec(mlp_spec), 
      dtype(dtype_), device(device_)
  {
    reset();
  }

  void reset() {
    // Unregister old “layers” if it exists
    try {
      unregister_module("layers");
    } catch (const c10::Error&) {
      // expected on first call
    }
  
    // Build a fresh Sequential already on the right device/dtype
    auto seq = torch::nn::Sequential();
    layers = register_module("layers", std::move(seq));
  
    auto dims = parse_mlp_spec(embedding_dim, mlp_spec);
    for (size_t i = 0; i + 1 < dims.size(); ++i) {
      bool last = (i + 2 == dims.size());
  
      if (!last) {
        // 1) Linear
        auto lin = torch::nn::Linear(torch::nn::LinearOptions(dims[i], dims[i+1]));
        lin->to(device, dtype); 
        layers->push_back(lin);
  
        // 2) BatchNorm
        auto bn = torch::nn::BatchNorm1d(torch::nn::BatchNorm1dOptions(dims[i+1]));
        bn->to(device, dtype);
        layers->push_back(bn);
  
        // 3) ReLU
        layers->push_back(torch::nn::ReLU(torch::nn::ReLUOptions().inplace(true)));
      } else {
        // Final layer, no bias
        auto lin = torch::nn::Linear(torch::nn::LinearOptions(dims[i], dims[i+1]).bias(false));
        lin->to(device, dtype);
        layers->push_back(lin);
      }
    }

    /* beeing efatic, move the module to device of type */
    layers->to(device, dtype);
    this->to(device, dtype);
  }

  torch::Tensor forward(torch::Tensor x) {
    /* x: [B, T, E] */
    const auto B = x.size(0);
    const auto T = x.size(1);

    /* 1. Flatten batch and time so features are the last dim. */
    x = x.reshape({B * T, x.size(-1)});          /* [B·T, E] */
    
    /* 2. MLP + BN work as‑is. */
    x = layers->forward(x);                      /* [B·T, E’] */
    
    /* 3. Restore time dimension. */
    x = x.view({B, T, x.size(-1)});              /* [B, T, E’] */

    return x;
  }

private:
  int                 embedding_dim;
  std::string         mlp_spec;
  torch::Dtype dtype;
  torch::Device device;

  torch::nn::Sequential layers{nullptr};

  static std::vector<int> parse_mlp_spec(int embed, const std::string& spec) {
    std::vector<int> out{embed};
    std::stringstream ss(spec);
    std::string tok;
    while (std::getline(ss, tok, '-'))
      out.push_back(std::stoi(tok));
    return out;
  }
};

TORCH_MODULE(VICReg_4D_Projector);

} /* namespace vicreg_4d */
} /* namespace wikimyei */
} /* namespace cuwacunu */
