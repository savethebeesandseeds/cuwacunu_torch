/* test_ts2vec_ECG200.cpp */
#include <optional>
#include <iostream>
#include <vector>
#include <torch/torch.h>
#include "wikimyei/heuristics/ts2vec/ts2vec.h"
#include "wikimyei/heuristics/ts2vec/datautils.h"
#include <filesystem>

inline void print_tensor_info(const torch::Tensor& tensor, const std::string& name = "tensor") {
    std::cout << name 
        << " | dtype: "  << tensor.dtype()
        << " | shape: "  << tensor.sizes()
        << " | device: " << tensor.device()
        << std::endl;
}

int main() {
    // -----------------------------------------------------
    // 0) Set Seed and Device
    // -----------------------------------------------------
    torch::manual_seed(42);
    torch::Device device(torch::cuda::is_available() ? torch::kCUDA : torch::kCPU);
    std::cout << "Using device: " << device << std::endl;

    // -----------------------------------------------------
    // 1) Set some variables
    // -----------------------------------------------------
    int max_train_length = 3000;
    int batch_size=32;

    // -----------------------------------------------------
    // 2) Load the Data
    // -----------------------------------------------------
    cuwacunu::wikimyei::ts2vec::UCRDataset dataset = cuwacunu::wikimyei::ts2vec::load_UCR("ECG200"); // [B, T, C]
    // no need to clean the test_data, mask takes care of this : dataset.test_data  = cuwacunu::wikimyei::ts2vec::clean_data(dataset.test_data,  max_train_length);
    dataset.train_data = cuwacunu::wikimyei::ts2vec::clean_data(dataset.train_data, max_train_length);
    
    print_tensor_info(dataset.test_data, "dataset.test_data");
    print_tensor_info(dataset.train_data, "dataset.test_data");

    // -----------------------------------------------------
    // 3) Create the train dataloader
    // -----------------------------------------------------
    
    using train_Dl = std::unique_ptr<
    torch::data::StatelessDataLoader<
        torch::data::datasets::MapDataset<
            torch::data::datasets::TensorDataset, 
            torch::data::transforms::Stack<torch::data::Example<torch::Tensor, void>>
        >,torch::data::samplers::RandomSampler>>;

    auto train_dataset = torch::data::datasets::TensorDataset(dataset.train_data)
        .map(torch::data::transforms::Stack<torch::data::Example<torch::Tensor, void>>());
    
    torch::data::samplers::RandomSampler loader_sampler(train_dataset.size().value());
    torch::data::DataLoaderOptions loader_options(
        std::min<int64_t>(batch_size, train_dataset.size().value())
    );
    loader_options.drop_last(true);
    
    auto train_dataloader = torch::data::make_data_loader(
        std::move(train_dataset),
        loader_sampler,
        loader_options
    );

    // -----------------------------------------------------
    // 4) Create the eval dataloader
    // -----------------------------------------------------

    // using test_Dl = std::unique_ptr<
    // torch::data::StatelessDataLoader<
    //     torch::data::datasets::MapDataset<
    //         torch::data::datasets::TensorDataset, 
    //         torch::data::transforms::Stack<torch::data::Example<torch::Tensor, void>>
    //     >,torch::data::samplers::SequentialSampler>>;
    
    // auto test_dataset = torch::data::datasets::TensorDataset(dataset.test_data)
    //     .map(torch::data::transforms::Stack<torch::data::Example<torch::Tensor, void>>());
    
    // test_Dl test_dataloader = torch::data::make_data_loader(
    //     std::move(test_dataset), 
    //     torch::data::samplers::SequentialSampler(test_dataset.size().value()), 
    //     torch::data::DataLoaderOptions(eff_batch_size)
    // );
  
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
        /* batch_size */batch_size,
        /* max_train_length */max_train_length,
        /* temporal_unit */0,
        /* default_encoder_mask_mode */ cuwacunu::wikimyei::ts2vec::TSEncoder_MaskMode_e::Binomial,
        /* pad_mask */ c10::nullopt,
        /* enable_buffer_averaging_ */ false
    );

    // -----------------------------------------------------
    // 4) Train
    // -----------------------------------------------------
    std::cout << ">>> Training in C++:\n";
    auto loss_log = model.fit<train_Dl>(
        /* dataloader */ train_dataloader, 
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
