/* test_vicreg_4d_observation_pipeline.cpp */
#include <torch/torch.h>
#include <torch/autograd.h>
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

#include "wikimyei/heuristics/VicReg/vicreg_4d.h"

int main() {
    torch::autograd::AnomalyMode::set_enabled(true);
    /* types definition */
    // using T = cuwacunu::camahjucunu::exchange::kline_t;
    using Td = cuwacunu::camahjucunu::exchange::basic_t;
    using Q = cuwacunu::camahjucunu::data::MemoryMappedConcatDataset<Td>;
    using K = cuwacunu::camahjucunu::data::observation_sample_t;
    using SeqSampler = torch::data::samplers::SequentialSampler;
    using RandSamper = torch::data::samplers::RandomSampler;

    /* set the test variables */
    const char* config_folder = "/cuwacunu/src/config/";
    std::string INSTRUMENT = "UTILITIES";
    std::string output_file = "/cuwacunu/src/tests/build/vicreg_4d_BTC_output.csv";

    // std::string INSTRUMENT = "BTCUSDT";
    int NUM_EPOCHS = 20;
    int NUM_ITERS  = 1;
    std::size_t BATCH_SIZE = 1;
    std::size_t dataloader_workers = 0;
    
    // auto device = torch::kCPU;
    // -----------------------------------------------------
    // 0) Set Seed and Device
    // -----------------------------------------------------
    torch::manual_seed(42);
    auto dtype = torch::kFloat32;
    auto device = torch::cuda::is_available() ? torch::kCUDA : torch::kCPU;
    // auto device = torch::kCPU;
    torch::cuda::synchronize();
    std::cout << "Using device: " << device << std::endl;

    /* read the config */
    TICK(read_config_);
    cuwacunu::piaabo::dconfig::config_space_t::change_config_file(config_folder);
    cuwacunu::piaabo::dconfig::config_space_t::update_config();
    PRINT_TOCK_ms(read_config_);

    /* read the instruction and create the observation pipeline */
    TICK(read_instruction_);
    std::string instruction = cuwacunu::piaabo::dconfig::config_space_t::observation_pipeline_instruction();
    auto obsPipe = cuwacunu::camahjucunu::BNF::observationPipeline();
    cuwacunu::camahjucunu::BNF::observation_instruction_t obsInst = obsPipe.decode(instruction);
    PRINT_TOCK_ms(read_instruction_);

    /* create the dataloader */
    TICK(create_dataloader_);
    auto training_data_loader = cuwacunu::camahjucunu::data::create_memory_mapped_dataloader<Q, K, Td, RandSamper>(
        /* instrument */ INSTRUMENT, 
        /* obs_inst */ obsInst, 
        /* force_binarization */ false, 
        /* batch_size */ BATCH_SIZE, 
        /* workers */ dataloader_workers
    );
    PRINT_TOCK_ms(create_dataloader_);

    
    /* model definition */
    // -----------------------------------------------------
    // 1) Set some variables
    // -----------------------------------------------------
    printf("training_data_loader.C_: %ld \n", training_data_loader.C_);
    printf("training_data_loader.T_: %ld \n", training_data_loader.T_);
    printf("training_data_loader.D_: %ld \n", training_data_loader.D_);
    // [B,T,C]

    // -----------------------------------------------------
    // 3) Instantiate VICReg_4d
    // -----------------------------------------------------
    std::cout << "Initializing the VICReg encoder...\n";
    TICK(Initialize_Model);
    cuwacunu::wikimyei::vicreg_4d::VICReg_4D model(
        training_data_loader.C_,     /* C_ */ 
        training_data_loader.T_,     /* T_ */ 
        training_data_loader.D_,     /* D_ */ 
        72,                 /* encoding_dims_*/
        64,                 /* channel_expansion_dim_*/
        32,                 /* fused_feature_dim_*/
        24,                 /* encoder_hidden_dims_*/
        10,                 /* encoder_depth_*/
        "128-256-218",      /* projector_mlp_spec_ */
        25.0,               /* sim_coeff_ */
        25.0,               /* std_coeff_ */
        1.0,                /* cov_coeff_ */
        0.001,              /* lr_ */
        dtype,              /* device_ */
        device,             /* device_ */
        false               /* enable_buffer_averaging_ */
    );
    PRINT_TOCK_ms(Initialize_Model);
    
    
    // -----------------------------------------------------
    // 4) Train (Fit)
    // -----------------------------------------------------
    std::cout << "Training the VICReg encoder...\n";
    TICK(Train_Model);
    std::vector<double> training_losses = model.fit<Q, K, Td>(
        training_data_loader, 
        NUM_EPOCHS, /* n_epochs */
        NUM_ITERS, /* n_iters */
        1000,       /* swa_start_iter */
        true        /* verbose */
    );
    PRINT_TOCK_ms(Train_Model);


    // // -----------------------------------------------------
    // // 5) Encode (Forward)
    // // -----------------------------------------------------
    // std::cout << "Beging the training...\n";
    // TICK(Train_Model);
    // auto loss_log = model.fit<train_Dl>(
    //     /* dataloader */ train_dataloader, 
    //     /* n_epochs */ 64,
    //     /* n_iters */ -1,
    //     /* verbose */true);
        
    // PRINT_TOCK_ms(Train_Model);
    // -----------------------------------------------------
    // Finalize
    // -----------------------------------------------------
    std::cout << "\nC++ Run Finished.\n";
    
    return 0;
}
    

    // std::cout << "Final C++ losses: [ ";
    // for (const auto &lossVal : loss_log) { // Use const auto&
    //     std::cout << lossVal << ",  ";
    // }
    // std::cout << "]" << std::endl;
    
