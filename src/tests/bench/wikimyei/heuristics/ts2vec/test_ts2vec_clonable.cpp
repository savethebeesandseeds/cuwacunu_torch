#include <torch/torch.h>
#include <iostream>
#include <vector>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>   // For module name
#include <typeinfo> // For typeid

#include "wikimyei/heuristics/ts2vec/dilated_conv.h"
#include "wikimyei/heuristics/ts2vec/encoder.h"

// Define the simplest possible Cloneable module inline
// --- Inherits from torch::nn::Cloneable ---
struct SimpleCloneableImpl : public torch::nn::Cloneable<SimpleCloneableImpl> {
    torch::nn::Linear lin{nullptr}; // Contains a Linear layer (like in previous errors)
    int in_feat, out_feat;          // Store dims needed for reset

    // Constructor: Store dimensions and CALL reset()
    SimpleCloneableImpl(int in_features = 5, int out_features = 2)
        : in_feat(in_features), out_feat(out_features)
    {
        // According to Cloneable pattern, initialization should happen in reset()
        reset();
    }

    // --- Implement the mandatory reset() method for Cloneable ---
    void reset() override {
        // Register submodules/parameters ONLY here
        lin = register_module("layer", torch::nn::Linear(in_feat, out_feat));
        // NOTE: Do NOT call lin->reset() within this reset(). The clone
        //       mechanism handles recursive initialization/resetting.
    }

    // Minimal forward method
    torch::Tensor forward(torch::Tensor x) {
        TORCH_CHECK(lin, "Linear layer 'lin' not initialized. Did reset() run?");
        return lin(x);
    }
};
// Create the wrapper class 'SimpleCloneable' using the macro
TORCH_MODULE(SimpleCloneable);


// Generic function to test clonability of a module
// Template arguments:
//   ModelWrapper: The TORCH_MODULE wrapper type (e.g., TSEncoder)
//   ModelImpl   : The implementation struct type (e.g., TSEncoderImpl)
// Arguments:
//   module_name : A string name for logging purposes
//   model1      : A const reference to the pre-constructed module instance to clone
// Returns:
//   true if clone test passes, false otherwise
template <typename ModelWrapper, typename ModelImpl>
bool test_module_clonability(const std::string& module_name, const ModelWrapper& model1) {
    std::cout << "\n--- Testing Clonability for: " << module_name << " ---" << std::endl;
    bool success = true; // Assume success
    ModelWrapper model2{nullptr}; // Wrapper for the clone

    // Determine device from model1's parameters or buffers
    torch::Device device = torch::kCPU;
    auto params_for_device = model1->parameters();
    if (!params_for_device.empty()) {
            device = params_for_device[0].device();
    } else {
            auto buffers_for_device = model1->buffers();
            if (!buffers_for_device.empty()) {
                device = buffers_for_device[0].device();
            }
    }
    std::cout << "  Using device: " << device << std::endl;
    std::cout << "  Attempting model1->clone()..." << std::endl;

    try {
        // 1. Call clone()
        std::shared_ptr<torch::nn::Module> cloned_base_ptr = model1->clone();
        TORCH_CHECK(cloned_base_ptr, "model1->clone() returned a null pointer for ", module_name);

        // 2. Downcast to specific Impl pointer
        std::shared_ptr<ModelImpl> model2_impl_ptr =
            std::dynamic_pointer_cast<ModelImpl>(cloned_base_ptr);
        TORCH_CHECK(model2_impl_ptr, "dynamic_pointer_cast to ", typeid(ModelImpl).name(), " failed after clone() for ", module_name);

        // 3. Create wrapper for the clone
        model2 = ModelWrapper(model2_impl_ptr);
        std::cout << "  SUCCESS! clone() call and casting completed." << std::endl;

        // 4. Verification
        std::cout << "  Verifying clone..." << std::endl;
        TORCH_CHECK(model2, "Cloned model wrapper (model2) is null/invalid for ", module_name);
        model2->to(device); // Ensure clone is on the same device

        auto params1 = model1->parameters();
        auto params2 = model2->parameters();
        TORCH_CHECK(params1.size() == params2.size(),
            "Parameter count mismatch for ", module_name, ": ", params1.size(), " vs ", params2.size());
        std::cout << "    Parameter count matches (" << params1.size() << ")." << std::endl;

        bool params_ok = true;
        if (params1.empty()) {
            std::cout << "    Model has no parameters to compare." << std::endl;
        } else {
            for (size_t i = 0; i < params1.size(); ++i) {
                if (params1[i].data_ptr() == params2[i].data_ptr()) {
                    std::cerr << "    VERIFICATION FAILURE: Parameter " << i << " shares memory (data_ptr() is identical)!" << std::endl;
                    params_ok = false;
                }
                // Use tolerance for floating point comparisons
                if (!torch::allclose(params1[i], params2[i], /*rtol=*/1e-05, /*atol=*/1e-08)) {
                     std::cerr << "    VERIFICATION FAILURE: Parameter " << i << " values differ!" << std::endl;
                    params_ok = false;
                }
            }
            if (params_ok) {
                std::cout << "    Parameters verified: Distinct tensors with equal values." << std::endl;
            } else {
                 success = false;
            }
        }
        // Note: Could add buffer verification here too if needed

    } catch (const c10::Error& e) {
        std::cerr << "  CLONE FAILURE (c10::Error) for " << module_name << ": " << e.what() << std::endl;
        if (std::string(e.what()).find("Parameter 'weight' already defined") != std::string::npos) {
             std::cerr << "    >>> Potential 'weight already defined' error! Check reset() implementation. <<<" << std::endl;
        }
         if (std::string(e.what()).find("clone() has not been implemented") != std::string::npos) {
             std::cerr << "    >>> Did you forget to inherit " << module_name << "Impl from torch::nn::Cloneable<...> ? <<<" << std::endl;
        }
        success = false;
    } catch (const std::exception& e) {
        std::cerr << "  CLONE FAILURE (std::exception) for " << module_name << ": " << e.what() << std::endl;
        success = false;
    }

    std::cout << "--- Clonability Test for " << module_name << " Finished ---" << std::endl;
    if (!success) {
        std::cerr << "*** Test FAILED for " << module_name << "! ***\n" << std::endl;
    } else {
        std::cout << "*** Test PASSED for " << module_name << "! ***\n" << std::endl;
    }
    return success;
}



using namespace cuwacunu::wikimyei::ts2vec; // Use your namespace

// --- Template function test_module_clonability defined above ---

int main() {
    torch::Device device(torch::kCPU); // Or torch::kCUDA if available
    bool all_tests_passed = true;
    int overall_return_code = 0;

    std::cout << "=============================================" << std::endl;
    std::cout << "Starting Clonability Tests for Custom Modules" << std::endl;
    std::cout << "=============================================" << std::endl;

    // --- 0. Test SimpleCloneable ---
    std::cout << "\n>>> Testing SimpleCloneable <<<\n";
    try {
            int in_features = 5; 
            int out_features = 2;
            SimpleCloneable simpl_model(in_features, out_features); // Create instance
            simpl_model->to(device);                       // Move to device
            // Call the test function
            if (!test_module_clonability<SimpleCloneable, SimpleCloneableImpl>("SimpleCloneable", simpl_model)) {
                all_tests_passed = false;
            }
    } catch (const std::exception& e) {
            std::cerr << "ERROR during SimpleCloneable test setup: " << e.what() << std::endl;
            all_tests_passed = false;
    }

    // --- 1. Test SamePadConv ---
    std::cout << "\n>>> Testing SamePadConv <<<\n";
    try {
            int in_c1=3, out_c1=8, k1=3, d1=1;
            SamePadConv spc_model(in_c1, out_c1, k1, d1); // Create instance
            spc_model->to(device);                       // Move to device
            // Call the test function
            if (!test_module_clonability<SamePadConv, SamePadConvImpl>("SamePadConv", spc_model)) {
                all_tests_passed = false;
            }
    } catch (const std::exception& e) {
            std::cerr << "ERROR during SamePadConv test setup: " << e.what() << std::endl;
            all_tests_passed = false;
    }

    // --- 2. Test ConvBlock ---
    std::cout << "\n>>> Testing ConvBlock <<<\n";
    try {
        int in_c2=8, out_c2=16, k2=3, d2=2; bool final2=true;
        ConvBlock cb_model(in_c2, out_c2, k2, d2, final2); // Create instance
        cb_model->to(device);                            // Move to device
        // Call the test function
        if (!test_module_clonability<ConvBlock, ConvBlockImpl>("ConvBlock", cb_model)) {
                all_tests_passed = false;
            }
    } catch (const std::exception& e) {
            std::cerr << "ERROR during ConvBlock test setup: " << e.what() << std::endl;
            all_tests_passed = false;
    }

    // --- 3. Test DilatedConvEncoder ---
    std::cout << "\n>>> Testing DilatedConvEncoder <<<\n";
    try {
            int in_c3=3, k3=3;
            // Use a local vector for safety when passing to constructor
            std::vector<int> channels3 = {8, 8, 16};
            DilatedConvEncoder dce_model(in_c3, channels3, k3); // Create instance
            dce_model->to(device);                             // Move to device
            // Call the test function
            if (!test_module_clonability<DilatedConvEncoder, DilatedConvEncoderImpl>("DilatedConvEncoder", dce_model)) {
                all_tests_passed = false;
            }
    } catch (const std::exception& e) {
            std::cerr << "ERROR during DilatedConvEncoder test setup: " << e.what() << std::endl;
            all_tests_passed = false;
    }


    // --- 4. Test TSEncoder ---
    std::cout << "\n>>> Testing TSEncoder <<<\n";
    try {
        int in_c4=3, out_c4=32, h4=16, depth4=2; std::string mode4="binomial";
        TSEncoder tse_model(in_c4, out_c4, h4, depth4, mode4); // Create instance
        tse_model->to(device);                                // Move to device
        // Call the test function
        if (!test_module_clonability<TSEncoder, TSEncoderImpl>("TSEncoder", tse_model)) {
            all_tests_passed = false;
        }
    } catch (const std::exception& e) {
            std::cerr << "ERROR during TSEncoder test setup: " << e.what() << std::endl;
            all_tests_passed = false;
    }


    // --- Final Summary ---
    std::cout << "\n=============================================" << std::endl;
    if (all_tests_passed) {
            std::cout << "All module clonability tests PASSED!" << std::endl;
            overall_return_code = 0;
    } else {
            std::cerr << "One or more module clonability tests FAILED!" << std::endl;
            overall_return_code = 1;
    }
    std::cout << "=============================================" << std::endl;
    return overall_return_code;
}