/* test_memory_mapped_dataset.cpp */
#include <torch/torch.h>
#include "piaabo/torch_compat/torch_utils.h"
#include "piaabo/dutils.h"
#include "piaabo/dconfig.h"
#include "piaabo/dfiles.h"
#include "camahjucunu/exchange/exchange_utils.h"
#include "camahjucunu/exchange/exchange_types_data.h"
#include "camahjucunu/exchange/exchange_types_enums.h"

#include "camahjucunu/data/memory_mapped_dataset.h"
// #include "camahjucunu/exchange/binance/binance_mech_data.h"

int main() {
    
    try {
        std::cout << std::fixed << std::setprecision(0);

        const std::string csv_filename("/cuwacunu/data/raw/BTCUSDT/1h/BTCUSDT-1h-all-years.csv");
        const std::string bin_filename("/cuwacunu/data/raw/BTCUSDT/1h/BTCUSDT-1h-all-years.bin");
        size_t buffer_size = 1024;
        char delimiter = ',';

        /* binarize the csv file */
        // TICK(csvFile_to_binary_);
        // cuwacunu::piaabo::dfiles::csvFile_to_binary<
        //     cuwacunu::camahjucunu::exchange::kline_t>(csv_filename, bin_filename, buffer_size, delimiter);
        // PRINT_TOCK_ns(csvFile_to_binary_);

        /* create the dataset for kline_t */
        TICK(MapMemory_);
        auto dataset = cuwacunu::camahjucunu::data::MemoryMappedDataset<
            cuwacunu::camahjucunu::exchange::kline_t>(bin_filename);
        PRINT_TOCK_ns(MapMemory_);

        // Get the tensor at a specific index
        TICK(GET_1);
        auto __ = dataset.get(100);
        PRINT_TOCK_ns(GET_1);

        // Get the tensor closest to a specific key value (e.g., close_time)
        std::vector<int64_t> targets = {
            1578567599999,
            1693407599999,
            1693407599990,  /* non existent value */
            1722470399999,
            1722470399990,  /* non existent value */
            1725148799999,
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
            cuwacunu::camahjucunu::data::observation_sample_t sample = dataset.get_by_key_value(target_close_time);
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
            cuwacunu::camahjucunu::data::observation_sample_t sample = dataset.get_sequence_ending_at_key_value(target_close_time, 5);
            PRINT_TOCK_ns(GET_SEQUENCE_BY_KEY_1);
            std::cout << "\t\t\t sample.features.shape(): "; for (const auto& dim : sample.features.sizes()) { std::cout << dim << " "; } std::cout << std::endl;
            std::cout << "\t\t\t sample.mask.shape(): "; for (const auto& dim : sample.mask.sizes()) { std::cout << dim << " "; } std::cout << std::endl;
            // std::cout << "\t sample.features.close_time: " << target_close_time << "=?" << sample.features.index({0,6}).item<double>() << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
