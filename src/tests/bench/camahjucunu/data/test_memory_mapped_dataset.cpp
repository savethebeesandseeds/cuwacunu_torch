/* test_memory_mapped_dataset.cpp */
#include <torch/torch.h>
#include "piaabo/torch_compat/torch_utils.h"
#include "piaabo/dutils.h"
#include "piaabo/dconfig.h"
#include "piaabo/dlarge_files.h"
#include "camahjucunu/exchange/exchange_utils.h"
#include "camahjucunu/exchange/exchange_types_data.h"
#include "camahjucunu/exchange/exchange_types_enums.h"

#include "camahjucunu/data/memory_mapped_dataset.h"
// #include "camahjucunu/exchange/binance/binance_mech_data.h"

int main() {
    
    try {
        std::cout << std::fixed << std::setprecision(0);

        const std::string csv_filename("/cuwacunu/data/BTCUSDT/1h/BTCUSDT-1h-all-years.csv");
        const std::string bin_filename("/cuwacunu/data/BTCUSDT/1h/BTCUSDT-1h-all-years.bin");
        size_t buffer_size = 1024;
        char delimiter = ',';

        /* binarize the csv file */
        TICK(csv_to_binary_);
        cuwacunu::piaabo::dlarge_files::csv_to_binary<
            cuwacunu::camahjucunu::exchange::kline_t>(csv_filename, bin_filename, buffer_size, delimiter);
        PRINT_TOCK_ns(csv_to_binary_);

        /* create the dataset for kline_t */
        TICK(MapMemory_);
        auto dataset = cuwacunu::camahjucunu::data::MemoryMappedDataset<
            cuwacunu::camahjucunu::exchange::kline_t>(bin_filename);
        PRINT_TOCK_ns(MapMemory_);

        // Get the tensor at a specific index
        TICK(GET_1);
        torch::Tensor tensor = dataset.get(100);
        PRINT_TOCK_ns(GET_1);

        // Get the tensor closest to a specific key value (e.g., close_time)
        std::vector<int64_t> targets = {
            1577740399999,  /* outside from below */
            1577840399999,
            1577969999999,
            1578315599999,
            1578567599999,
            1693407599999,
            1693407599990,  /* non existent value */
            1722470399999,
            1722470399990,  /* non existent value */
            1725148799999,
            1693407599999,  /* go back again */
            1577840399999,  /* go back again */
            1725548799999   /* outside from avobe */
        };
        for(int64_t target_close_time: targets) {
            TICK(GET_BY_KEY_1);
            torch::Tensor tensor_by_date = dataset.get_by_key_value(static_cast<int64_t>(target_close_time));
            PRINT_TOCK_ns(GET_BY_KEY_1);
            std::cout << "tensor_by_date.close_time: " << target_close_time << "=?" << tensor_by_date[6].item<double>() << std::endl;
        }

        log_info("-- -- -- -- -- -- -- -- \n");
        dataset.get_sequence_ending_at_key_value(1693407599999, 5);

        // std::cout << "tensor_by_date: " << tensor_by_date << std::endl;

    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
