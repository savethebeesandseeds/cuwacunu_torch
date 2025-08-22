#include <torch/torch.h>
#include <iostream>
#include <vector>   // Required for state_dict type
#include <string>   // Required for mask_mode
#include <exception> // Required for std::exception

// Include your encoder header relative to this test file's location
// Adjust the path if necessary
#include "wikimyei/heuristics/ts2vec/ts2vec_encoder.h"

// Assuming the necessary types are in this namespace
using namespace cuwacunu::wikimyei::ts2vec;

void compare_tsencoder(int input_dims, int output_dims, int hidden_dims, int depth, std::string mask_mode, int batch_size, int seq_len) {
    torch::manual_seed(42);

    cuwacunu::wikimyei::ts2vec::TSEncoder encoder(input_dims, output_dims, hidden_dims, depth, mask_mode);
    encoder->train();

    auto x = torch::linspace(-1, 1, batch_size * seq_len * input_dims);
    x = x.view({batch_size, seq_len, input_dims});

    std::cout << "Input tensor shape: " << x.sizes() << "\n";
    std::cout << "Input tensor:\n" << x << "\n";

    auto encoded = encoder->forward(x, "all_true");

    std::cout << "Encoded output shape: " << encoded.sizes() << "\n";
    std::cout << "Encoded output tensor:\n" << encoded << "\n";
}

void test_tsencoder(int input_dims, int output_dims, int hidden_dims, int depth, std::string mask_mode, int batch_size, int seq_len) {
    torch::manual_seed(42);
    // Use CPU for this test for simplicity, unless CUDA is essential for the error
    torch::Device device(torch::kCPU);
    std::cout << "Using device: " << device << std::endl;

    std::cout << "\n--- Creating Encoder Instance ---\n";
    TSEncoder encoder(input_dims, output_dims, hidden_dims, depth, mask_mode);
    encoder->to(device); // Move model to device
    encoder->train();    // Set to train mode (might affect dropout, etc.)
    std::cout << "Encoder instance created.\n";

    // --- Test state_dict() Access ---
    std::cout << "\n--- Testing state_dict() Access Methods ---\n";
    torch::OrderedDict<std::string, torch::Tensor> state_dict_result;
    bool sd_retrieved = false;

    // Method 1: Access via wrapper -> operator
    try {
        std::cout << "[Test 1] Attempting: encoder->state_dict()" << std::endl;
        state_dict_result = encoder->state_dict();
        std::cout << "  [Test 1] SUCCESS!" << std::endl;
        sd_retrieved = true;
    } catch (const c10::Error& e) {
        std::cerr << "  [Test 1] FAILURE (c10::Error): " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "  [Test 1] FAILURE (std::exception): " << e.what() << std::endl;
    }

    // Method 2: Access directly on wrapper object
    if (!sd_retrieved) {
        try {
            std::cout << "[Test 2] Attempting: encoder.state_dict()" << std::endl;
            state_dict_result = encoder.state_dict();
            std::cout << "  [Test 2] SUCCESS!" << std::endl;
            sd_retrieved = true;
        } catch (const c10::Error& e) {
            std::cerr << "  [Test 2] FAILURE (c10::Error): " << e.what() << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "  [Test 2] FAILURE (std::exception): " << e.what() << std::endl;
        }
    }

    // Method 3: Access via Impl reference with scope resolution
    if (!sd_retrieved) {
        try {
            std::cout << "[Test 3] Attempting: (*encoder).torch::nn::Module::state_dict()" << std::endl;
            TSEncoderImpl& impl_ref = *encoder; // Get reference to Impl
            state_dict_result = impl_ref.torch::nn::Module::state_dict();
            std::cout << "  [Test 3] SUCCESS!" << std::endl;
            sd_retrieved = true;
        } catch (const c10::Error& e) {
            std::cerr << "  [Test 3] FAILURE (c10::Error): " << e.what() << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "  [Test 3] FAILURE (std::exception): " << e.what() << std::endl;
        }
    }

    if (!sd_retrieved) {
        std::cerr << "\nERROR: Failed to retrieve state_dict using any method. Cannot test load_state_dict.\n";
        return; // Exit if we couldn't get a state dict
    } else {
         std::cout << "\nSuccessfully retrieved state_dict. Number of entries: " << state_dict_result.size() << std::endl;
    }


    // --- Test load_state_dict() Access ---
    std::cout << "\n--- Testing load_state_dict() Access Methods ---\n";
    // Create a second encoder instance to load into
    TSEncoder encoder2(input_dims, output_dims, hidden_dims, depth, mask_mode);
    encoder2->to(device);
    bool sd_loaded = false;

    // Method 1: Access via wrapper -> operator
    try {
        std::cout << "[Test 1] Attempting: encoder2->load_state_dict()" << std::endl;
        encoder2->load_state_dict(state_dict_result);
        std::cout << "  [Test 1] SUCCESS!" << std::endl;
        sd_loaded = true;
    } catch (const c10::Error& e) {
        std::cerr << "  [Test 1] FAILURE (c10::Error): " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "  [Test 1] FAILURE (std::exception): " << e.what() << std::endl;
    }

    // Method 2: Access directly on wrapper object
    if (!sd_loaded) {
        try {
            std::cout << "[Test 2] Attempting: encoder2.load_state_dict()" << std::endl;
            encoder2.load_state_dict(state_dict_result);
            std::cout << "  [Test 2] SUCCESS!" << std::endl;
            sd_loaded = true;
        } catch (const c10::Error& e) {
            std::cerr << "  [Test 2] FAILURE (c10::Error): " << e.what() << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "  [Test 2] FAILURE (std::exception): " << e.what() << std::endl;
        }
    }

    // Method 3: Access via Impl reference with scope resolution
    if (!sd_loaded) {
        try {
            std::cout << "[Test 3] Attempting: (*encoder2).torch::nn::Module::load_state_dict()" << std::endl;
            TSEncoderImpl& impl_ref2 = *encoder2; // Get reference to Impl
            impl_ref2.torch::nn::Module::load_state_dict(state_dict_result);
            std::cout << "  [Test 3] SUCCESS!" << std::endl;
            sd_loaded = true;
        } catch (const c10::Error& e) {
            std::cerr << "  [Test 3] FAILURE (c10::Error): " << e.what() << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "  [Test 3] FAILURE (std::exception): " << e.what() << std::endl;
        }
    }

     if (!sd_loaded) {
        std::cerr << "\nERROR: Failed to load state_dict using any method.\n";
    } else {
         std::cout << "\nSuccessfully loaded state_dict into second encoder.\n";
    }

    // --- Optional: Test parameters() access ---
     std::cout << "\n--- Testing parameters() Access Method ---\n";
     try {
         std::cout << "[Test 1] Attempting: encoder->parameters()" << std::endl;
         auto params = encoder->parameters();
         std::cout << "  [Test 1] SUCCESS! Found " << params.size() << " parameter tensors." << std::endl;
     } catch (const c10::Error& e) {
          std::cerr << "  [Test 1] FAILURE (c10::Error): " << e.what() << std::endl;
     } catch (const std::exception& e) {
         std::cerr << "  [Test 1] FAILURE (std::exception): " << e.what() << std::endl;
     }


} // End test_tsencoder

int main() {
    compare_tsencoder(
        /* input_dims */ 3,
        /* output_dims */ 4,
        /* hidden_dims */ 8,
        /* depth */ 3,
        /* mask_mode */ "binomial",
        /* batch_size */ 2,
        /* seq_len */ 5);
        
    test_tsencoder(
        /* input_dims */ 3,
        /* output_dims */ 4,
        /* hidden_dims */ 8,
        /* depth */ 3,
        /* mask_mode */ "binomial",
        /* batch_size */ 2,
        /* seq_len */ 5);
    return 0;
}