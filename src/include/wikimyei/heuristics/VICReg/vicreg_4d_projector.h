#include <torch/torch.h>
#include <vector>
#include <string>
#include <sstream>

namespace cuwacunu {
namespace wikimyei {
namespace vicreg_4d {
    
/**
 * @class VICReg_4D_ProjectorImpl
 * @brief A dynamic multi-layer perceptron (MLP) used as a projection head in VICReg.
 *
 * This module constructs a sequence of linear layers, batch normalizations, and ReLU activations
 * based on a string specification of layer sizes. It is designed to transform high-dimensional
 * embeddings (e.g., from a ResNet encoder) into a space suitable for self-supervised contrastive loss.
 *
 * Example:
 *   embedding_dim = 2048
 *   mlp_spec = "8192-8192-8192"
 *   → Resulting architecture: 2048 → 8192 → 8192 → 8192
 *
 * Architecture:
 *   - All but the final layer: Linear + BatchNorm1d + ReLU (in-place)
 *   - Final layer: Linear (no bias)
 *
 * Usage:
 *   Projector projector(embedding_dim, mlp_spec);
 *   torch::Tensor out = projector->forward(input_tensor);
 *
 * This structure follows the VICReg design, where the projection head is critical for decorrelating
 * features and ensuring meaningful invariance and variance in representations.
 */

class VICReg_4D_ProjectorImpl : public torch::nn::Module {
public:
    VICReg_4D_ProjectorImpl(int embedding_dim, const std::string& mlp_spec) {
        // Parse the MLP specification string: "8192-8192-8192"
        std::vector<int> layer_dims = parse_mlp_spec(embedding_dim, mlp_spec);

        // Build layers dynamically based on dimensions
        for (size_t i = 0; i < layer_dims.size() - 2; ++i) {
            layers->push_back(torch::nn::Linear(layer_dims[i], layer_dims[i + 1]));
            layers->push_back(torch::nn::BatchNorm1d(layer_dims[i + 1]));
            layers->push_back(torch::nn::ReLU(true));
        }

        // Final linear layer (no bias)
        layers->push_back(torch::nn::Linear(torch::nn::LinearOptions(layer_dims[layer_dims.size() - 2],
                                                                     layer_dims.back()).bias(false)));

        // Register the sequential module
        register_module("layers", layers);
    }

    torch::Tensor forward(torch::Tensor x) {
        return layers->forward(x);
    }

private:
    torch::nn::Sequential layers{nullptr};

    // Helper function to parse the mlp_spec string
    static std::vector<int> parse_mlp_spec(int embedding_dim, const std::string& spec) {
        std::vector<int> dims;
        dims.push_back(embedding_dim);

        std::stringstream ss(spec);
        std::string dim;
        while (std::getline(ss, dim, '-')) {
            dims.push_back(std::stoi(dim));
        }
        return dims;
    }
};

TORCH_MODULE(VICReg_4D_Projector);  // Defines Projector as a shared_ptr<VICReg_4D_ProjectorImpl>

} /* namespace vicreg_4d */
} /* namespace wikimyei */
} /* namespace cuwacunu */
