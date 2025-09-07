/* test_memory_mapped_dataset.cpp */
#include <torch/torch.h>
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
#include "camahjucunu/BNF/implementations/observation_pipeline/observation_pipeline.h"
// #include "camahjucunu/exchange/binance/binance_mech_data.h"

int main() {
    
    /* read the config */
    const char* config_folder = "/cuwacunu/src/config/";
    cuwacunu::piaabo::dconfig::config_space_t::change_config_file(config_folder);
    cuwacunu::piaabo::dconfig::config_space_t::update_config();

    /* set the test variables */
    // std::string INSTRUMENT = "BTCUSDT";
    std::string INSTRUMENT = "UTILITIES";
    std::string instruction = cuwacunu::piaabo::dconfig::config_space_t::observation_pipeline_instruction();
    
    /* create the observation pipeline */
    auto obsPipe = cuwacunu::camahjucunu::BNF::observationPipeline();
    cuwacunu::camahjucunu::observation_instruction_t decoded_data = obsPipe.decode(instruction);

    /* create the dataset */
    using T = cuwacunu::camahjucunu::exchange::basic_t;
    // using T = cuwacunu::camahjucunu::exchange::kline_t;

    auto concat_dataset =
        cuwacunu::camahjucunu::data::create_memory_mapped_concat_dataset<T>(
            INSTRUMENT, decoded_data, /* force_binarization */ false
        );

    /* produce the test tensor closest to a specific key value (e.g., close_time) */
    std::vector<int64_t> targets = {
        1693407599999,
        1693407599990,  /* non existent value */
        1722470399999,
        1722470399990,  /* non existent value */
        1725148799999,
        1725159599999,
        1725548799999   /* outside from avobe */
    };

    /* --- --- --- --- --- --- --- --- --- --- --- */
    /*            test get_by_key_value            */
    /* --- --- --- --- --- --- --- --- --- --- --- */
    log_info("-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- \n");
    log_info("--             get_by_key_value              -- \n");
    log_info("-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- \n");
    for(int64_t target_close_time: targets) {
        TICK(GET_BY_KEY_);
        cuwacunu::camahjucunu::data::observation_sample_t sample = concat_dataset.get_by_key_value(target_close_time);
        PRINT_TOCK_ns(GET_BY_KEY_);
        std::cout << "\t\t\t sample.features.shape(): "; for (const auto& dim : sample.features.sizes()) { std::cout << dim << " "; } std::cout << std::endl;
        std::cout << "\t\t\t sample.mask.shape(): "; for (const auto& dim : sample.mask.sizes()) { std::cout << dim << " "; } std::cout << std::endl;
        // std::cout << "\t sample.features.close_time: " << target_close_time << "=?" << sample.features.index({0,6}).item<double>() << std::endl;
    }

    log_info("-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- \n");
    log_info("--     get_sequence_ending_at_key_value      -- \n");
    log_info("-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- \n");
    /* --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- */
    /*            test get_sequence_ending_at_key_value            */
    /* --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- */
    
    for(int64_t target_close_time: targets) {
        TICK(GET_SEQUENCE_BY_KEY_1);
        cuwacunu::camahjucunu::data::observation_sample_t sample = concat_dataset.get_sequence_ending_at_key_value(target_close_time);
        PRINT_TOCK_ns(GET_SEQUENCE_BY_KEY_1);
        std::cout << "\t\t\t sample.features.shape(): "; for (const auto& dim : sample.features.sizes()) { std::cout << dim << " "; } std::cout << std::endl;
        std::cout << "\t\t\t sample.mask.shape(): "; for (const auto& dim : sample.mask.sizes()) { std::cout << dim << " "; } std::cout << std::endl;
        
        std::cout << "--- --- --- Channel: " << "target_close_time: " << cuwacunu::piaabo::unixTimeToString(static_cast<long>(target_close_time / 1000)) << std::endl;
        for (int i = 0; i < sample.features.size(0); ++i) {
            std::cout << "Slice " << i << ":\n";
            for (int j = 0; j < sample.features.size(1); ++j) {
                long mask_value = sample.mask[i][j].item<long>();
                std::cout << "  Row: " << j << " Mask: " << mask_value << " : \t";
                if(mask_value == 0) { std::cout << ANSI_COLOR_Dim_Red; } else { std::cout << ANSI_COLOR_Dim_Green; }
                for (int k = 0; k < sample.features.size(2); ++k) {
                    float value = sample.features[i][j][k].item<float>();
                    std::cout << std::setw(18) << std::fixed << std::setprecision(4) << value;
                }
                std::cout << ANSI_COLOR_RESET;
                std::cout << '\n';
            }
            std::cout << '\n';
        }
    }

    return 0;
}
