/* test_vicreg_4d_train.cpp */
#include <torch/torch.h>
#include <torch/autograd.h>
#include <fstream>
#include <iostream>
#include <filesystem>
#include "piaabo/torch_compat/torch_utils.h"
#include "piaabo/dutils.h"
#include "piaabo/dconfig.h"
#include "piaabo/dfiles.h"
#include "camahjucunu/types/types_utils.h"
#include "camahjucunu/types/types_data.h"
#include "camahjucunu/types/types_enums.h"

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
    using Datatype_t = cuwacunu::camahjucunu::exchange::kline_t;    // cuwacunu::camahjucunu::exchange::basic_t;
    using Dataset_t = cuwacunu::camahjucunu::data::MemoryMappedConcatDataset<Datatype_t>;
    using DataSample_t = cuwacunu::camahjucunu::data::observation_sample_t;
    // using RandSamper = torch::data::samplers::RandomSampler;
    using SeqSampler = torch::data::samplers::SequentialSampler;

    TICK(create_dataloader_);
    auto training_data_loader = cuwacunu::camahjucunu::data::make_obs_pipeline_mm_dataloader
        <Datasample_t, Sampler_t>(std::string_view instrument);
    PRINT_TOCK_ms(create_dataloader_);

    // -----------------------------------------------------
    // Instantiate VICReg_4d (model definition)
    // -----------------------------------------------------
    std::cout << "Initializing the VICReg encoder...\n";
    TICK(Initialize_Model);
    cuwacunu::wikimyei::vicreg_4d::VICReg_4D model(
        "VICReg_representation",        /* component name */
        training_data_loader.C_,        /* C */ 
        training_data_loader.T_,        /* T */ 
        training_data_loader.D_         /* D */ 
    );
    PRINT_TOCK_ms(Initialize_Model);
    
    // -----------------------------------------------------
    // 4) Train (Fit)
    // -----------------------------------------------------
    std::cout << "Training the VICReg encoder...\n";
    TICK(Train_Model);
    auto training_losses = model.fit<Dataset_t, DataSample_t, Datatype_t>(
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