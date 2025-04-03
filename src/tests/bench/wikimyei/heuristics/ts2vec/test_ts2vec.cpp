#include <optional>
#include <iostream>
#include <vector>
#include <torch/torch.h>
#include "wikimyei/heuristics/ts2vec/ts2vec.h" // Includes all necessary class definitions
#include <filesystem>
int main() {
    // -----------------------------------------------------
    // 1) Set Seed and Device
    // -----------------------------------------------------
    torch::manual_seed(42);
    torch::Device device(torch::kCPU); // Match Python test device
    std::cout << "Using device: " << device << std::endl;

    // -----------------------------------------------------
    // 2) Create Training Data
    // -----------------------------------------------------
    auto train_data = torch::ones({5, 10, 1}).to(device); // Keep on target device

    // -----------------------------------------------------
    // 3) Instantiate TS2Vec in C++
    // -----------------------------------------------------
    cuwacunu::wikimyei::ts2vec::TS2Vec model(
        /* input_dims */1,
        /* output_dims */320,
        /* hidden_dims */64,
        /* depth */10,
        /* device */device, // Use defined device
        /* lr */0.001,
        /* batch_size */2,
        /* max_train_length */std::nullopt,
        /* temporal_unit */0,
        /* encoder_mask_mode */ "binomial",
        /* enable_buffer_averaging_ */ false
    );
    // Model should be on the specified device from constructor/internal calls

    // -----------------------------------------------------
    // 4) Train for 2 epochs
    // -----------------------------------------------------
    std::cout << ">>> Training in C++:\n";
    auto loss_log = model.fit(
        /* train_data */ train_data,
        /* n_epochs */ 2,
        /* n_iters */ -1,
        /* verbose */true);

    std::cout << "Final C++ losses: [ ";
    for (const auto &lossVal : loss_log) { // Use const auto&
        std::cout << lossVal << " ";
    }
    std::cout << "]" << std::endl;

    // --- Ensure model is in eval mode for consistent inference ---
    // Note: encode() method should internally handle setting eval mode
    // model.eval(); // Usually done within encode

    // -----------------------------------------------------
    // 5) Create Control Input for Inference Test
    // -----------------------------------------------------
    std::cout << "\n>>> Running C++ Inference Test <<<\n";
    // Use linspace for deterministic input different from training data
    auto test_input = torch::linspace(-0.5, 0.5, /*steps=*/5 * 10 * 1)
                          .reshape({5, 10, 1})
                          .to(device); // Ensure it's on the correct device
    std::cout << "Test input shape: " << test_input.sizes() << std::endl;

    // -----------------------------------------------------
    // 6) Run Inference (Encode)
    // -----------------------------------------------------
    torch::Tensor cpp_output;
    try {
        // Use default mask ('all_true' used implicitly by encode if mask not passed)
        // Ensure encode returns tensor on CPU for saving/comparison ease
        cpp_output = model.encode(test_input).cpu(); // Run encode and move result to CPU
        std::cout << "C++ Encode successful. Output shape: " << cpp_output.sizes() << std::endl;

        // -----------------------------------------------------
        // 7) Save C++ Output Tensor to File
        // -----------------------------------------------------
        const std::string output_filename = "cpp_output.pt";
        std::cout << "cpp_output defined: " << cpp_output.defined() << std::endl;
        std::cout << "cpp_output dtype: " << cpp_output.dtype() << std::endl;
        std::cout << "cpp_output sizes: " << cpp_output.sizes() << std::endl;
        torch::save(cpp_output, output_filename);
        std::cout << "C++ output tensor saved to " << output_filename << std::endl;

    } catch (const std::exception& e) {
         std::cerr << "ERROR during C++ inference or saving: " << e.what() << std::endl;
         return 1; // Indicate error
    }

    std::cout << "\nC++ Run Finished.\n";
    return 0;
}