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

#include "camahjucunu/data/observation_sample.h"
#include "camahjucunu/data/memory_mapped_dataset.h"
#include "camahjucunu/data/memory_mapped_datafile.h"
#include "camahjucunu/data/memory_mapped_dataloader.h"
#include "camahjucunu/BNF/implementations/observation_pipeline/observation_pipeline.h"

#include "wikimyei/heuristics/representation_learning/VICReg/vicreg_4d.h"

int main() {
    // torch::autograd::AnomalyMode::set_enabled(true);
    torch::autograd::AnomalyMode::set_enabled(false);
    torch::globalContext().setBenchmarkCuDNN(true);
    torch::globalContext().setDeterministicCuDNN(false);
    WARM_UP_CUDA();

    /* set the test variables */
    const char* config_folder = "/cuwacunu/src/config/";

    /* read the config */
    TICK(read_config_);
    cuwacunu::piaabo::dconfig::config_space_t::change_config_file(config_folder);
    cuwacunu::piaabo::dconfig::config_space_t::update_config();
    PRINT_TOCK_ms(read_config_);
    
    // -----------------------------------------------------
    // Create the Dataloader
    // -----------------------------------------------------
    torch::manual_seed(cuwacunu::piaabo::dconfig::config_space_t::get<int>("GENERAL", "torch_seed"));

    /* types definition */
    std::string INSTRUMENT = "BTCUSDT";                     // "UTILITIES"
    using Td = cuwacunu::camahjucunu::exchange::kline_t;    // cuwacunu::camahjucunu::exchange::basic_t;
    using Q = cuwacunu::camahjucunu::data::MemoryMappedConcatDataset<Td>;
    using K = cuwacunu::camahjucunu::data::observation_sample_t;
    // using RandSamper = torch::data::samplers::RandomSampler;
    using SeqSampler = torch::data::samplers::SequentialSampler;

    TICK(create_dataloader_);
    auto training_data_loader = cuwacunu::camahjucunu::data::create_memory_mapped_dataloader<Q, K, Td, SeqSampler>(
        INSTRUMENT,                                                                                             /* instrument */
        cuwacunu::camahjucunu::BNF::observationPipeline()
            .decode(cuwacunu::piaabo::dconfig::config_space_t::observation_pipeline_instruction()),             /* obs_inst */ 
        cuwacunu::piaabo::dconfig::config_space_t::get<bool>    ("DATA_LOADER", "dataloader_force_binarization"),    /* force_binarization */
        cuwacunu::piaabo::dconfig::config_space_t::get<int>     ("DATA_LOADER", "dataloader_batch_size"),            /* batch_size */
        cuwacunu::piaabo::dconfig::config_space_t::get<int>     ("DATA_LOADER", "dataloader_workers")                /* workers */
    );
    PRINT_TOCK_ms(create_dataloader_);

    // -----------------------------------------------------
    // Instantiate VICReg_4d (model definition)
    // -----------------------------------------------------
    std::cout << "Initializing the VICReg encoder...\n";
    TICK(Initialize_Model);
    cuwacunu::wikimyei::vicreg_4d::VICReg_4D model(
        training_data_loader.C_,        /* C */ 
        training_data_loader.T_,        /* T */ 
        training_data_loader.D_,        /* D */ 
        "VICReg_representation"         /* Component name */
    );
    PRINT_TOCK_ms(Initialize_Model);
    
    // -----------------------------------------------------
    // 4) Train (Fit)
    // -----------------------------------------------------
    std::cout << "Training the VICReg encoder...\n";
    TICK(Train_Model);
    auto training_losses = model.fit<Q, K, Td>(
        training_data_loader, 
        cuwacunu::piaabo::dconfig::config_space_t::get<int>     ("VICReg",  "n_epochs"),          /* n_epochs */
        cuwacunu::piaabo::dconfig::config_space_t::get<int>     ("VICReg",  "n_iters"),           /* n_iters */
        cuwacunu::piaabo::dconfig::config_space_t::get<int>     ("VICReg",  "swa_start_iter"),    /* swa_start_iter */
        cuwacunu::piaabo::dconfig::config_space_t::get<bool>    ("VICReg",  "verbose_train")      /* verbose */
    );
    PRINT_TOCK_ms(Train_Model);

    // -----------------------------------------------------
    // 5) Save (Model)
    // -----------------------------------------------------
    model.save(cuwacunu::piaabo::dconfig::config_space_t::get("VICReg", "model_path"));
    // -----------------------------------------------------
    // Finalize
    // -----------------------------------------------------
    std::cout << "\n Observation Pipeline test Finished.\n";
    
    return 0;
}
    // std::cout << "Final C++ losses: [ ";
    // for (const auto &lossVal : loss_log) { // Use const auto&
    //     std::cout << lossVal << ",  ";
    // }
    // std::cout << "]" << std::endl;
    
