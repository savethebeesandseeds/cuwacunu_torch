/* test_vicreg_4d_train.cpp */
#include <torch/torch.h>
#include <torch/autograd.h>
#include <algorithm>
#include <cctype>
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
#include "camahjucunu/dsl/observation_pipeline/observation_spec.h"

#include "wikimyei/representation/VICReg/vicreg_4d.h"

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
    cuwacunu::iitepi::config_space_t::change_config_file(config_folder);
    cuwacunu::iitepi::config_space_t::update_config();
    PRINT_TOCK_ms(read_config_);

    const auto contract_hash =
        cuwacunu::iitepi::board_space_t::contract_hash_for_binding(
            cuwacunu::iitepi::config_space_t::locked_board_hash(),
            cuwacunu::iitepi::config_space_t::locked_board_binding_id());
    cuwacunu::iitepi::contract_space_t::assert_intact_or_fail_fast(contract_hash);
    {
        std::string configured_device =
            cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash)->get<std::string>("VICReg", "device");
        std::transform(configured_device.begin(),
                       configured_device.end(),
                       configured_device.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (!torch::cuda::is_available() &&
            (configured_device == "gpu" || configured_device == "cuda")) {
            std::cout << "[skip] VICReg device is configured as GPU but CUDA is unavailable\n";
            return 0;
        }
    }
    
    // -----------------------------------------------------
    // Create the Dataloader
    // -----------------------------------------------------
    torch::manual_seed(cuwacunu::iitepi::config_space_t::get<int>("GENERAL", "torch_seed"));

    /* types definition */
    std::string INSTRUMENT = "BTCUSDT";                     // "UTILITIES"
    using Datatype_t = cuwacunu::camahjucunu::exchange::kline_t;    // cuwacunu::camahjucunu::exchange::basic_t;
    using Dataset_t = cuwacunu::camahjucunu::data::MemoryMappedConcatDataset<Datatype_t>;
    using DataSample_t = cuwacunu::camahjucunu::data::observation_sample_t;
    // using RandSamper = torch::data::samplers::RandomSampler;
    using Sampler_t = torch::data::samplers::SequentialSampler;

    TICK(create_dataloader_);
    auto training_data_loader = cuwacunu::camahjucunu::data::make_obs_mm_dataloader
        <Datatype_t, Sampler_t>(INSTRUMENT, contract_hash);
    PRINT_TOCK_ms(create_dataloader_);

    // -----------------------------------------------------
    // Instantiate VICReg_4d (model definition)
    // -----------------------------------------------------
    std::cout << "Initializing the VICReg encoder...\n";
    TICK(Initialize_Model);
    cuwacunu::wikimyei::vicreg_4d::VICReg_4D model(
        contract_hash,
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
    const int configured_epochs = cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash)->get<int>("VICReg", "n_epochs", -1);
    const int configured_iters = cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash)->get<int>("VICReg", "n_iters", -1);
    const int configured_swa_start = cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash)->get<int>("VICReg", "swa_start_iter");
    const int smoke_epochs = 1;
    const int smoke_iters = 1;
    const int smoke_swa_start = std::max(0, std::min(configured_swa_start, smoke_iters));
    std::cout << "[smoke] VICReg training limited to n_epochs=" << smoke_epochs
              << " n_iters=" << smoke_iters
              << " (configured " << configured_epochs << "/" << configured_iters << ")\n";

    auto training_losses = model.fit<Dataset_t, DataSample_t, Datatype_t>(
        training_data_loader, 
        smoke_epochs,                /* n_epochs */
        smoke_iters,                 /* n_iters */
        smoke_swa_start,             /* swa_start_iter */
        false                        /* verbose */
    );
    PRINT_TOCK_ms(Train_Model);

    // -----------------------------------------------------
    // 5) Save (Model)
    // -----------------------------------------------------
    model.save(cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash)->get<std::string>("VICReg", "model_path"));
    // -----------------------------------------------------
    // Finalize
    // -----------------------------------------------------
    std::cout << "\n Observation Pipeline test Finished.\n";
    
    return 0;
}
