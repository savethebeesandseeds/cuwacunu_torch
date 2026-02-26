#include <torch/torch.h>
#include <iostream>
#include <vector>
#include <exception>

// Define the simplest possible module inline
// Inherits publicly from torch::nn::Module
struct SimplestImpl : public torch::nn::Module {
    torch::nn::Linear lin{nullptr}; // Add one layer just to have parameters

    // Constructor initializes the layer (standard practice now without Cloneable/reset)
    SimplestImpl(int in_features=5, int out_features=2) {
        lin = register_module("layer", torch::nn::Linear(in_features, out_features));
    }

    // Minimal forward method
    torch::Tensor forward(torch::Tensor x) {
        return lin(x);
    }

    // NOTE: No state_dict, load_state_dict, parameters, etc. defined here.
    //       We rely entirely on public inheritance from torch::nn::Module.
};

// Create the wrapper class 'Simplest' using the macro
TORCH_MODULE(Simplest);

int main() {
    std::cout << "--- Running Simplest LibTorch Module Test ---" << std::endl;
    torch::Device device(torch::kCPU);
    std::cout << "Using device: " << device << std::endl;

    // Create instance of the wrapper
    std::cout << "Creating Simplest model instance..." << std::endl;
    Simplest model(5, 2); // Example sizes for the Linear layer
    model->to(device);
    std::cout << "Model created." << std::endl;
    int return_code = 0; // Track if any test fails

    // Test 1: state_dict() access via ->
    std::cout << "\nTesting model->state_dict()..." << std::endl;
    try {
        // This is the idiomatic way it *should* work
        auto sd = model->state_dict();
        std::cout << "  SUCCESS! Retrieved state_dict with " << sd.size() << " items." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "  FAILURE: " << e.what() << std::endl;
        return_code = 1; // Mark failure
    }

    // Test 2: parameters() access via ->
    std::cout << "\nTesting model->parameters()..." << std::endl;
    try {
        // This is the idiomatic way it *should* work
        auto params = model->parameters();
        std::cout << "  SUCCESS! Retrieved parameters. Count: " << params.size() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "  FAILURE: " << e.what() << std::endl;
        return_code = 1; // Mark failure
    }

    // Test 3: named_parameters() access via ->
    std::cout << "\nTesting model->named_parameters()..." << std::endl;
    try {
        // This is the idiomatic way it *should* work
        auto named_params = model->named_parameters();
        std::cout << "  SUCCESS! Retrieved named_parameters. Count: " << named_params.size() << std::endl;
        for(const auto& p : named_params) {
             std::cout << "    " << p.key() << ": " << p.value().sizes() << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "  FAILURE: " << e.what() << std::endl;
        return_code = 1; // Mark failure
    }

    // Test 4: load_state_dict() (create dummy state dict first)
    std::cout << "\nTesting model->load_state_dict()..." << std::endl;
    try {
        // Create a dummy state dict (just for testing the call, not correctness of loading)
        torch::OrderedDict<std::string, torch::Tensor> dummy_sd;
        dummy_sd.insert("layer.weight", torch::randn({2, 5})); // Match constructor defaults
        dummy_sd.insert("layer.bias", torch::randn({2}));

        // This is the idiomatic way it *should* work
        model->load_state_dict(dummy_sd);
        std::cout << "  SUCCESS! Call to load_state_dict completed." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "  FAILURE: " << e.what() << std::endl;
        return_code = 1; // Mark failure
    }


    std::cout << "\n--- Simplest Module Test Finished ---" << std::endl;
    if (return_code != 0) {
        std::cerr << "\n*** One or more tests FAILED! ***" << std::endl;
    } else {
        std::cout << "\n*** All tested methods appear accessible. ***" << std::endl;
    }
    return return_code;
}