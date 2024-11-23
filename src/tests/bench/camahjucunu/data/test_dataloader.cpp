/*

{
    
    resolve the dilema of the separated projects cuwacunu_utils
    create the binary files if: do this every time the dataloader is created
        a) no file exist,
        b) csv file date exceeds binary file date
    
    # operate only if the daterange is requested
    
    # deal with holes in the data

    # libtorch

    # this one is a sub module of the upper observation_pipeline


class binance_dataloader : Dataloader {

    stream<sample_t> Access_Data_Lazily:
    sample_t Random_sample
}

}

# check if the data needs to be completed by requesting binance 

*/





/* test_binance_dataloader.cpp */
#include <torch/torch.h>
// #include <torch/data/dataloader.h>
// #include <torch/data/dataloader_options.h>
// #include <torch/data/samplers/sequential.h>
#include "piaabo/dutils.h"
#include "piaabo/dconfig.h"
#include "piaabo/dlarge_files.h"
#include "camahjucunu/exchange/exchange_utils.h"
#include "camahjucunu/exchange/exchange_types_data.h"
#include "camahjucunu/exchange/exchange_types_enums.h"

#include "camahjucunu/data/memory_mapped_dataset.h"
// #include "camahjucunu/exchange/binance/binance_mech_data.h"

int main() {
    // using DatasetType = cuwacunu::camahjucunu::data::MemoryMappedDataset<
    //     cuwacunu::camahjucunu::exchange::kline_t>;

    // // Create the dataset
    // auto dataset = DatasetType("data.bin");

    // // Define a custom collate function
    // auto collate_fn = [](std::vector<torch::Tensor> batch) {
    //     return torch::stack(batch);
    // };

    // // Create DataLoaderOptions
    // auto options = torch::data::DataLoaderOptions();
    // options.batch_size(64);
    // options.workers(4);

    // // Get dataset size
    // auto dataset_size = dataset.size().value();

    // // Create the sampler
    // auto sampler = torch::data::samplers::SequentialSampler(dataset_size);

    // // Create the DataLoader
    // using DataLoader = torch::data::StatelessDataLoader<
    //     DatasetType,
    //     torch::data::samplers::SequentialSampler
    // >;

    // auto data_loader = DataLoader(
    //     std::move(dataset),
    //     std::move(sampler),
    //     options
    // );

    // // Training loop
    // for (auto& batch : data_loader) {
    //     auto inputs = collate_fn(batch); // Apply the collate function
    //     // Use inputs in your algorithm
    //     std::cout << "Batch size: " << inputs.sizes() << std::endl;
    // }

    // return 0;


    cuwacunu::camahjucunu::exchange::kline_t record;
    record.tensor_features();

    const std::string csv_filename("/data/BTCUSDT/1h/BTCUSDT-1h-all-years.csv");
    const std::string bin_filename("/data/BTCUSDT/1h/BTCUSDT-1h-all-years.bin");
    size_t buffer_size = 1024;
    char delimiter = ',';

    TICK(csv_to_binary_);
    cuwacunu::piaabo::dlarge_files::csv_to_binary<
        cuwacunu::camahjucunu::exchange::kline_t>(csv_filename, bin_filename, buffer_size, delimiter);
    PRINT_TOCK_ns(csv_to_binary_);

    TICK(MapMemory_);
    auto dataset = cuwacunu::camahjucunu::data::MemoryMappedDataset<
        cuwacunu::camahjucunu::exchange::kline_t>(bin_filename);
    PRINT_TOCK_ns(MapMemory_);

    std::cout << "dataset size: " << dataset.size().value() << std::endl;

    std::cout << std::fixed << std::setprecision(0);

    TICK(GET_0);
    auto GET_0_v = dataset.get(0);
    PRINT_TOCK_ns(GET_0);
    std::cout << "GET_0_v: " << GET_0_v << std::endl;

    TICK(GET_1);
    auto GET_1_v = dataset.get(1);
    PRINT_TOCK_ns(GET_1);
    std::cout << "GET_1_v: " << GET_1_v << std::endl;

    TICK(GET_2);
    auto GET_2_v = dataset.get(2);
    PRINT_TOCK_ns(GET_2);
    std::cout << "GET_2_v: " << GET_2_v << std::endl;

    TICK(GET_3);
    auto GET_3_v = dataset.get(3);
    PRINT_TOCK_ns(GET_3);
    std::cout << "GET_3_v: " << GET_3_v << std::endl;


    TICK(GET_40000);
    auto GET_40000_v = dataset.get(40000);
    PRINT_TOCK_ns(GET_40000);
    std::cout << "GET_40000_v: " << GET_40000_v << std::endl;

    TICK(GET_40001);
    auto GET_40001_v = dataset.get(40001);
    PRINT_TOCK_ns(GET_40001);
    std::cout << "GET_40001_v: " << GET_40001_v << std::endl;

    TICK(GET_40002);
    auto GET_40002_v = dataset.get(40002);
    PRINT_TOCK_ns(GET_40002);
    std::cout << "GET_40002_v: " << GET_40002_v << std::endl;

    TICK(GET_40003);
    auto GET_40003_v = dataset.get(40003);
    PRINT_TOCK_ns(GET_40003);
    std::cout << "GET_40003_v: " << GET_40003_v << std::endl;


    TICK(GET_alt_0);
    auto GET_alt_0_v = dataset.get(0);
    PRINT_TOCK_ns(GET_alt_0);
    std::cout << "GET_alt_0_v: " << GET_alt_0_v << std::endl;

    TICK(GET_alt_1);
    auto GET_alt_1_v = dataset.get(1);
    PRINT_TOCK_ns(GET_alt_1);
    std::cout << "GET_alt_1_v: " << GET_alt_1_v << std::endl;

    TICK(GET_alt_2);
    auto GET_alt_2_v = dataset.get(2);
    PRINT_TOCK_ns(GET_alt_2);
    std::cout << "GET_alt_2_v: " << GET_alt_2_v << std::endl;

    TICK(GET_alt_3);
    auto GET_alt_3_v = dataset.get(3);
    PRINT_TOCK_ns(GET_alt_3);
    std::cout << "GET_alt_3_v: " << GET_alt_3_v << std::endl;


    TICK(GET_alt_40000);
    auto GET_alt_40000_v = dataset.get(40000);
    PRINT_TOCK_ns(GET_alt_40000);
    std::cout << "GET_alt_40000_v: " << GET_alt_40000_v << std::endl;

    TICK(GET_alt_40001);
    auto GET_alt_40001_v = dataset.get(40001);
    PRINT_TOCK_ns(GET_alt_40001);
    std::cout << "GET_alt_40001_v: " << GET_alt_40001_v << std::endl;

    TICK(GET_alt_40002);
    auto GET_alt_40002_v = dataset.get(40002);
    PRINT_TOCK_ns(GET_alt_40002);
    std::cout << "GET_alt_40002_v: " << GET_alt_40002_v << std::endl;

    TICK(GET_alt_40003);
    auto GET_alt_40003_v = dataset.get(40003);
    PRINT_TOCK_ns(GET_alt_40003);
    std::cout << "GET_alt_40003_v: " << GET_alt_40003_v << std::endl;


    return 0;
}



