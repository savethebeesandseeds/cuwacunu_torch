/* test_memory_mapped_dataset.cpp */
#include "piaabo/dutils.h"
#include "piaabo/dconfig.h"
#include "piaabo/dfiles.h"
#include "camahjucunu/exchange/exchange_utils.h"
#include "camahjucunu/exchange/exchange_types_data.h"
#include "camahjucunu/exchange/exchange_types_enums.h"

#include "camahjucunu/data/memory_mapped_datafile.h"
// #include "camahjucunu/exchange/binance/binance_mech_data.h"

int main() {
    
    /* define the variables */
    const std::string csv_filename("/cuwacunu/data/raw/BTCUSDT/30m/BTCUSDT-30m-all-years.csv");
    size_t buffer_size = 1024;
    char delimiter = ',';

    /* binarize the csv file */
    TICK(prepare_binary_file_from_csv_);
    std::string bin_filename = cuwacunu::camahjucunu::data::prepare_binary_file_from_csv<
        cuwacunu::camahjucunu::exchange::kline_t>(csv_filename, false, buffer_size, delimiter);
    PRINT_TOCK_ns(prepare_binary_file_from_csv_);

    return 0;
}
