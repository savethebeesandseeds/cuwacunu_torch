#include <optional>
#include <iostream>
#include <vector>
#include <torch/torch.h>
#include "wikimyei/heuristics/ts2vec/ts2vec.h"
#include "wikimyei/heuristics/ts2vec/datautils.h"
#include <filesystem>

inline void print_tensor_info(const torch::Tensor& tensor, const std::string& name = "tensor") {//waka
    std::cout << name 
        << " | dtype: "  << tensor.dtype()
        << " | shape: "  << tensor.sizes()
        << " | device: " << tensor.device()
        << std::endl;
}

int main() {
    // -----------------------------------------------------
    // 1) Set Seed and Device
    // -----------------------------------------------------
    torch::manual_seed(42);
    torch::Device device(torch::cuda::is_available() ? torch::kCUDA : torch::kCPU);
    std::cout << "Using device: " << device << std::endl;

    // -----------------------------------------------------
    // 2) Load the Data
    // -----------------------------------------------------
    cuwacunu::wikimyei::ts2vec::UCRDataset dataset = cuwacunu::wikimyei::ts2vec::load_UCR("ECG200");
    print_tensor_info(dataset.train_data, "dataset.train_data");//waka

    // -----------------------------------------------------
    // 3) Instantiate TS2Vec in C++
    // -----------------------------------------------------
    cuwacunu::wikimyei::ts2vec::TS2Vec model(
        /* input_dims */dataset.train_data.sizes().back(),
        /* output_dims */320,
        /* hidden_dims */64,
        /* depth */10,
        /* device */device, // Use defined device
        /* lr */0.001,
        /* batch_size */32,
        /* max_train_length */3000,
        /* temporal_unit */0,
        /* encoder_mask_mode */ "binomial",
        /* enable_buffer_averaging_ */ false
    );
    
    // -----------------------------------------------------
    // 4) Train
    // -----------------------------------------------------
    std::cout << ">>> Training in C++:\n";
    auto loss_log = model.fit(
        /* train_data */ dataset.train_data,
        /* n_epochs */ 64,
        /* n_iters */ -1,
        /* verbose */true);

    std::cout << "Final C++ losses: [ ";
    for (const auto &lossVal : loss_log) { // Use const auto&
        std::cout << lossVal << ",  ";
    }
    std::cout << "]" << std::endl;
    
    // -----------------------------------------------------
    // Finalize
    // -----------------------------------------------------
    std::cout << "\nC++ Run Finished.\n";

    return 0;
}
