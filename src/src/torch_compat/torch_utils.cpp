#include "torch_utils.h"

void validate_module_parameters(const torch::nn::Module& model) {
    for (const auto& named_param : model.named_parameters()) {
        const auto& name = named_param.key();
        const auto& param = named_param.value();
        TORCH_CHECK(param.defined(), "Parameter '" + name + "' is undefined.");
        TORCH_CHECK(!torch::isnan(param).any().item<bool>(), "Parameter '" + name + "' contains NaN.");
        TORCH_CHECK(param.numel() > 0, "Parameter '" + name + "' is empty.");
        std::cout << "Validated parameter '" << name << "', size: " << param.sizes() << std::endl;
    }
}
