/* test_memory_mapped_concat_dataloader.cpp */
#include <torch/torch.h>
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

int main() {
    
    /* read the config */
    const char* config_folder = "/cuwacunu/src/config/";
    cuwacunu::piaabo::dconfig::config_space_t::change_config_file(config_folder);
    cuwacunu::piaabo::dconfig::config_space_t::update_config();

    /* set the test variables */
    // std::string INSTRUMENT = "UTILITIES";
    std::string INSTRUMENT = "BTCUSDT";
    std::string instruction = cuwacunu::piaabo::dconfig::config_space_t::observation_pipeline_instruction();
    std::size_t batch_size = 64;
    std::size_t workers = 4;

    /* types definition */
    // using T = cuwacunu::camahjucunu::exchange::basic_t;
    using T = cuwacunu::camahjucunu::exchange::kline_t;
    using Q = cuwacunu::camahjucunu::data::MemoryMappedConcatDataset<T>;
    using K = cuwacunu::camahjucunu::data::observation_sample_t;
    
    
    /* create the observation pipeline */
    auto obsPipe = cuwacunu::camahjucunu::BNF::observationPipeline();
    cuwacunu::camahjucunu::BNF::observation_instruction_t decoded_data = obsPipe.decode(instruction);

    {
        using S = torch::data::samplers::SequentialSampler;
        /* create the dataloader */
        auto data_loader = cuwacunu::camahjucunu::data::create_memory_mapped_dataloader<Q, K, T, S>(
            /* instrument */                INSTRUMENT, 
            /* observation_instruction_t */ decoded_data, 
            /* force_binarization */        false, 
            /* batch_size */                batch_size, 
            /* num workers*/                workers);
        
        /* --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- */
        /*                  test sequential sampler                    */
        /* --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- */
        log_info("-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- \n");
        log_info("--            sequential sampler             -- \n");
        log_info("-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- \n");
        
        size_t count = 0;
        for (auto& sample_batch : data_loader) {
            count++;
            auto collacted_sample = K::collate_fn(sample_batch);
            std::cout << "\t\t\t collacted_sample.features.shape(): "; for (const auto& dim : collacted_sample.features.sizes()) { std::cout << dim << " "; } std::cout << std::endl;
            std::cout << "\t\t\t collacted_sample.mask.shape(): "; for (const auto& dim : collacted_sample.mask.sizes()) { std::cout << dim << " "; } std::cout << std::endl;
            // /* ---- first element in the batch ---- */
            // auto first_feat = collacted_sample.features.select(0, 0); // same as collacted_sample.features[0]
            // auto first_mask = collacted_sample.mask.select(0, 0);     // same as collacted_sample.mask[0]
    
            // std::cout << "\t\t\t first feature tensor:\n" << first_feat << '\n';
            // std::cout << "\t\t\t first mask tensor:\n"    << first_mask << '\n';
        }
        std::cout << "--- --- ---: " << count << std::endl;
    }
    
    /* ------------------- */

    {
        using S = torch::data::samplers::RandomSampler;
        /* create the dataloader */
        auto data_loader = cuwacunu::camahjucunu::data::create_memory_mapped_dataloader<Q, K, T, S>(
            /* instrument */                INSTRUMENT, 
            /* observation_instruction_t */ decoded_data, 
            /* force_binarization */        false, 
            /* batch_size */                batch_size, 
            /* num workers*/                workers);

        /* --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- */
        /*                  test random sampler                        */
        /* --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- */
        log_info("-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- \n");
        log_info("--            random sampler                 -- \n");
        log_info("-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- \n");
        
        size_t count = 0;
        for (auto& sample_batch : data_loader) {
            count++;
            auto collacted_sample = K::collate_fn(sample_batch);
            std::cout << "\t\t\t collacted_sample.features.shape(): "; for (const auto& dim : collacted_sample.features.sizes()) { std::cout << dim << " "; } std::cout << std::endl;
            std::cout << "\t\t\t collacted_sample.mask.shape(): "; for (const auto& dim : collacted_sample.mask.sizes()) { std::cout << dim << " "; } std::cout << std::endl;

        }
        std::cout << "--- --- ---: " << count << std::endl;
    }

    return 0;
}
