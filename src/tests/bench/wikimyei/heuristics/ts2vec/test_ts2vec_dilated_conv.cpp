// #include <torch/torch.h>
// #include <iostream>
// #include "wikimyei/heuristics/ts2vec/dilated_conv.h"

// // Helper to print a small slice of a tensor
// void print_tensor_slice(const torch::Tensor& tensor, int batch_idx = 0, int channel_idx = 0, int length = 5) {
//     auto slice = tensor[batch_idx][channel_idx].slice(0, 0, length);
//     std::cout << "  Tensor slice (batch " << batch_idx << ", channel " << channel_idx << "): [ ";
//     for (int i = 0; i < slice.size(0); ++i) {
//         std::cout << std::fixed << std::setprecision(4) << slice[i].item<float>() << " ";
//     }
//     std::cout << "]\n";
// }

// void run_test(int in_channels, int seq_len, int batch_size,
//               const std::vector<int>& channels, int kernel_size) {
//     std::cout << "\nRunning test: in_channels=" << in_channels
//               << ", seq_len=" << seq_len << ", batch_size=" << batch_size << "\n";
//     std::cout << "Channels: [ ";
//     for (int c : channels) std::cout << c << " ";
//     std::cout << "], Kernel Size: " << kernel_size << "\n";

//     torch::manual_seed(42);
//     // auto x = torch::randn({batch_size, in_channels, seq_len});
//     auto x = torch::linspace(0, 1, batch_size * in_channels * seq_len).reshape({batch_size, in_channels, seq_len});

//     std::cout << "\nInput Tensor Shape: " << x.sizes() << "\n";
//     print_tensor_slice(x);

//     cuwacunu::wikimyei::ts2vec::DilatedConvEncoder encoder(in_channels, channels, kernel_size);
//     encoder->eval(); 

//     for (size_t i = 0; i < encoder->net->size(); ++i) {
//         auto layer = encoder->net->ptr(i)->as<cuwacunu::wikimyei::ts2vec::ConvBlock>();
//         auto x_before = x.clone();
//         x = layer->forward(x);

//         std::cout << "\n[" << i << "] ConvBlock\n";
//         std::cout << "  Input Shape: " << x_before.sizes()
//                   << ", Output Shape: " << x.sizes() << "\n";
//         std::cout << "  Input Norm: " << x_before.norm().item<float>()
//                   << ", Output Norm: " << x.norm().item<float>() << "\n";
//     }

//     std::cout << "\nFinal Output Shape: " << x.sizes() << "\n";
//     print_tensor_slice(x);
//     std::cout << "Final Output Norm: " << x.norm().item<float>() << "\n";
// }

// int main() {
//     run_test(
//         /*in_channels=*/3,
//         /*seq_len=*/16,
//         /*batch_size=*/2,
//         /*channels=*/{8, 16, 32},
//         /*kernel_size=*/3
//     );
//     return 0;
// }


#include <torch/torch.h>
#include <iostream>
#include <vector>
#include <string>
#include <exception>

// Include your dilated_conv header relative to this test file's location
// Adjust the path if necessary
#include "wikimyei/heuristics/ts2vec/dilated_conv.h"

// Assuming the necessary types are in this namespace
using namespace cuwacunu::wikimyei::ts2vec;

// --- Test Function for SamePadConv ---
void test_samepadconv() {
    std::cout << "\n--- Testing SamePadConv ---" << std::endl;
    torch::manual_seed(1); // Consistent initialization
    torch::Device device(torch::kCPU); // Use CPU for simplicity

    // Module parameters
    int in_c = 3, out_c = 8, k = 3, d = 1, B = 2, T = 10;

    std::cout << "Creating SamePadConv(" << in_c << ", " << out_c << ", k=" << k << ", d=" << d << ")" << std::endl;
    SamePadConv model(in_c, out_c, k, d);
    model->to(device);

    // Input data (Batch, Channels, Time)
    auto x = torch::randn({B, in_c, T}, device);
    torch::OrderedDict<std::string, torch::Tensor> state_dict_result;
    bool sd_ok = false;

    // Test Forward
    try {
        std::cout << "  Attempting forward()..." << std::endl;
        auto y = model->forward(x);
        std::cout << "    Forward SUCCESS. Output shape: " << y.sizes() << std::endl;
        TORCH_CHECK(y.size(0) == B && y.size(1) == out_c && y.size(2) == T, "Output shape mismatch!");
    } catch (const std::exception& e) {
        std::cerr << "    Forward FAILED: " << e.what() << std::endl;
        return; // Stop test if forward fails
    }

    // Test state_dict access (Method 1: wrapper->state_dict())
    try {
        std::cout << "  Attempting model->state_dict()..." << std::endl;
        state_dict_result = model->state_dict();
        std::cout << "    state_dict() SUCCESS. Items: " << state_dict_result.size() << std::endl;
        sd_ok = true;
    } catch (const std::exception& e) {
        std::cerr << "    state_dict() FAILED: " << e.what() << std::endl;
    }

    // Test load_state_dict access (Method 1: wrapper->load_state_dict())
    if (sd_ok) {
        std::cout << "  Creating second SamePadConv instance..." << std::endl;
        SamePadConv model2(in_c, out_c, k, d);
        model2->to(device);
        try {
            std::cout << "  Attempting model2->load_state_dict()..." << std::endl;
            model2->load_state_dict(state_dict_result);
            std::cout << "    load_state_dict() SUCCESS." << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "    load_state_dict() FAILED: " << e.what() << std::endl;
        }
    }
    std::cout << "--- SamePadConv Test Finished ---" << std::endl;
}

// --- Test Function for ConvBlock ---
void test_convblock() {
    std::cout << "\n--- Testing ConvBlock ---" << std::endl;
    torch::manual_seed(2);
    torch::Device device(torch::kCPU);

    // Module parameters (testing with projection)
    int in_c = 8, out_c = 16, k = 3, d = 2, B = 2, T = 15;
    bool final = true;

    std::cout << "Creating ConvBlock(" << in_c << ", " << out_c << ", k=" << k << ", d=" << d << ", final=" << final << ")" << std::endl;
    ConvBlock model(in_c, out_c, k, d, final);
    model->to(device);

    // Input data (Batch, Channels, Time)
    auto x = torch::randn({B, in_c, T}, device);
    torch::OrderedDict<std::string, torch::Tensor> state_dict_result;
    bool sd_ok = false;

    // Test Forward
    try {
        std::cout << "  Attempting forward()..." << std::endl;
        auto y = model->forward(x);
        std::cout << "    Forward SUCCESS. Output shape: " << y.sizes() << std::endl;
         TORCH_CHECK(y.size(0) == B && y.size(1) == out_c && y.size(2) == T, "Output shape mismatch!");
    } catch (const std::exception& e) {
        std::cerr << "    Forward FAILED: " << e.what() << std::endl;
        return;
    }

    // Test state_dict access (Method 1: wrapper->state_dict())
    try {
        std::cout << "  Attempting model->state_dict()..." << std::endl;
        state_dict_result = model->state_dict();
        std::cout << "    state_dict() SUCCESS. Items: " << state_dict_result.size() << std::endl;
        sd_ok = true;
    } catch (const std::exception& e) {
        std::cerr << "    state_dict() FAILED: " << e.what() << std::endl;
    }

    // Test load_state_dict access (Method 1: wrapper->load_state_dict())
    if (sd_ok) {
        std::cout << "  Creating second ConvBlock instance..." << std::endl;
        ConvBlock model2(in_c, out_c, k, d, final);
        model2->to(device);
        try {
            std::cout << "  Attempting model2->load_state_dict()..." << std::endl;
            model2->load_state_dict(state_dict_result);
            std::cout << "    load_state_dict() SUCCESS." << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "    load_state_dict() FAILED: " << e.what() << std::endl;
        }
    }
    std::cout << "--- ConvBlock Test Finished ---" << std::endl;
}

// --- Test Function for DilatedConvEncoder ---
void test_dilatedconvencoder() {
    std::cout << "\n--- Testing DilatedConvEncoder ---" << std::endl;
    torch::manual_seed(3);
    torch::Device device(torch::kCPU);

    // Module parameters
    int in_c = 3, k = 3, B = 2, T = 20;
    std::vector<int> channels = {8, 8, 16};

    std::cout << "Creating DilatedConvEncoder(" << in_c << ", {";
    for(size_t i=0; i<channels.size(); ++i) std::cout << channels[i] << (i==channels.size()-1 ? "" : ", ");
    std::cout << "}, k=" << k << ")" << std::endl;
    DilatedConvEncoder model(in_c, channels, k);
    model->to(device);

    // Input data (Batch, Channels, Time)
    auto x = torch::randn({B, in_c, T}, device);
    torch::OrderedDict<std::string, torch::Tensor> state_dict_result;
    bool sd_ok = false;

    // Test Forward
    try {
        std::cout << "  Attempting forward()..." << std::endl;
        auto y = model->forward(x);
        std::cout << "    Forward SUCCESS. Output shape: " << y.sizes() << std::endl;
         TORCH_CHECK(y.size(0) == B && y.size(1) == channels.back() && y.size(2) == T, "Output shape mismatch!");
    } catch (const std::exception& e) {
        std::cerr << "    Forward FAILED: " << e.what() << std::endl;
        return;
    }

    // Test state_dict access (Method 1: wrapper->state_dict())
    try {
        std::cout << "  Attempting model->state_dict()..." << std::endl;
        state_dict_result = model->state_dict();
        std::cout << "    state_dict() SUCCESS. Items: " << state_dict_result.size() << std::endl;
        sd_ok = true;
    } catch (const std::exception& e) {
        std::cerr << "    state_dict() FAILED: " << e.what() << std::endl;
    }

    // Test load_state_dict access (Method 1: wrapper->load_state_dict())
    if (sd_ok) {
        std::cout << "  Creating second DilatedConvEncoder instance..." << std::endl;
        DilatedConvEncoder model2(in_c, channels, k);
        model2->to(device);
        try {
            std::cout << "  Attempting model2->load_state_dict()..." << std::endl;
            model2->load_state_dict(state_dict_result);
            std::cout << "    load_state_dict() SUCCESS." << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "    load_state_dict() FAILED: " << e.what() << std::endl;
        }
    }
    std::cout << "--- DilatedConvEncoder Test Finished ---" << std::endl;
}


int main() {
    // Ensure the modules included from dilated_conv.h do NOT use Cloneable or reset()
    std::cout << "Starting Dilated Conv Module Tests..." << std::endl;
    test_samepadconv();
    test_convblock();
    test_dilatedconvencoder();
    std::cout << "\nAll Dilated Conv Module Tests Finished." << std::endl;
    return 0;
}

