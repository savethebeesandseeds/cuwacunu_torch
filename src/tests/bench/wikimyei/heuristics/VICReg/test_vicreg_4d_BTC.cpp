/* test_ts2vec_BTC.cpp */
#include <torch/torch.h>
#include <fstream>
#include <iostream>
#include <filesystem>
#include "piaabo/torch_compat/torch_utils.h"
#include "piaabo/dutils.h"
#include "piaabo/dconfig.h"
#include "piaabo/dfiles.h"
#include "camahjucunu/exchange/exchange_utils.h"
#include "camahjucunu/exchange/exchange_types_data.h"
#include "camahjucunu/exchange/exchange_types_enums.h"

#include "camahjucunu/data/memory_mapped_dataset.h"
#include "camahjucunu/data/memory_mapped_datafile.h"
#include "camahjucunu/data/memory_mapped_dataloader.h"
#include "camahjucunu/BNF/implementations/observation_pipeline/observation_pipeline.h"

#include "wikimyei/heuristics/ts2vec_4d/ts2vec_4d.h"
#include "wikimyei/heuristics/ts2vec_4d/datautils.h"

inline void print_tensor_info(const torch::Tensor& tensor, const std::string& name = "tensor") {
    std::cout << name 
        << " | dtype: "  << tensor.dtype()
        << " | shape: "  << tensor.sizes()
        << " | device: " << tensor.device()
        << std::endl;
}

int main() {
    /* types definition */
    // using T = cuwacunu::camahjucunu::exchange::kline_t;
    using T = cuwacunu::camahjucunu::exchange::basic_t;
    using Q = cuwacunu::camahjucunu::data::MemoryMappedConcatDataset<T>;
    using K = cuwacunu::camahjucunu::data::observation_sample_t;
    using SeqSampler = torch::data::samplers::SequentialSampler;
    using RandSamper = torch::data::samplers::RandomSampler;

    /* set the test variables */
    const char* config_folder = "/cuwacunu/src/config/";
    std::string INSTRUMENT = "UTILITIES";
    std::string output_file = "/cuwacunu/src/tests/build/ts2vect_BTC_output.csv";

    // std::string INSTRUMENT = "BTCUSDT";
    int NUM_EPOCHS = 20;
    std::size_t BATCH_SIZE = 12;
    std::size_t dataloader_workers = 1;
    
    // auto device = torch::kCPU;
    // -----------------------------------------------------
    // 0) Set Seed and Device
    // -----------------------------------------------------
    torch::manual_seed(42);
    auto device = torch::cuda::is_available() ? torch::kCUDA : torch::kCPU;
    std::cout << "Using device: " << device << std::endl;

    /* read the config */
    TICK(read_config_);
    cuwacunu::piaabo::dconfig::config_space_t::change_config_file(config_folder);
    cuwacunu::piaabo::dconfig::config_space_t::update_config();
    PRINT_TOCK_ns(read_config_);

    /* read the instruction and create the observation pipeline */
    TICK(read_instruction_);
    std::string instruction = cuwacunu::piaabo::dconfig::config_space_t::observation_pipeline_instruction();
    auto obsPipe = cuwacunu::camahjucunu::BNF::observationPipeline();
    cuwacunu::camahjucunu::BNF::observation_instruction_t obsInst = obsPipe.decode(instruction);
    PRINT_TOCK_ns(read_instruction_);

    /* create the dataloader */
    TICK(create_dataloader_);
    auto data_loader = cuwacunu::camahjucunu::data::create_memory_mapped_dataloader<Q, K, T, RandSamper>(
        /* instrument */ INSTRUMENT, 
        /* obs_inst */ obsInst, 
        /* force_binarization */ false, 
        /* batch_size */ BATCH_SIZE, 
        /* workers */ dataloader_workers
    );
    PRINT_TOCK_ns(create_dataloader_);

    /* model definition */
    
    

    // -----------------------------------------------------
    // 1) Set some variables
    // -----------------------------------------------------
    int batch_size=32;
    printf("data_loader.C_: %ld \n", data_loader.C_);
    printf("data_loader.T_: %ld \n", data_loader.T_);
    printf("data_loader.D_: %ld \n", data_loader.D_);

    // [B,T,C]
    // -----------------------------------------------------
    // 3) Instantiate TS2Vec in C++
    // -----------------------------------------------------
    // cuwacunu::wikimyei::ts2vec::TS2Vec model(
    //     /* input_dims */dataset.train_data.sizes().back(),
    //     /* output_dims */320,
    //     /* hidden_dims */64,
    //     /* depth */10,
    //     /* device */device, // Use defined device
    //     /* lr */0.001,
    //     /* batch_size */batch_size,
    //     /* temporal_unit */0,
    //     /* default_encoder_mask_mode */ cuwacunu::wikimyei::ts2vec::VICRegEncoder_MaskMode_e::Binomial,
    //     /* pad_mask */ c10::nullopt,
    //     /* enable_buffer_averaging_ */ false
    // );

    // // -----------------------------------------------------
    // // 4) Train
    // // -----------------------------------------------------
    // std::cout << ">>> Training in C++:\n";
    // auto loss_log = model.fit<train_Dl>(
    //     /* dataloader */ train_dataloader, 
    //     /* n_epochs */ 64,
    //     /* n_iters */ -1,
    //     /* verbose */true);

    // std::cout << "Final C++ losses: [ ";
    // for (const auto &lossVal : loss_log) { // Use const auto&
    //     std::cout << lossVal << ",  ";
    // }
    // std::cout << "]" << std::endl;
    
    // // -----------------------------------------------------
    // // Finalize
    // // -----------------------------------------------------
    // std::cout << "\nC++ Run Finished.\n";

    return 0;
}
