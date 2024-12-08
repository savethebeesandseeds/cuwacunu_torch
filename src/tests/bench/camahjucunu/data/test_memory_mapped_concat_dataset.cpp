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
#include "camahjucunu/data/memory_mapped_datafile.h"
#include "camahjucunu/BNF/implementations/observation_pipeline/observation_pipeline.h"
// #include "camahjucunu/exchange/binance/binance_mech_data.h"

int main() {
    
    /* start the variables */
    // std::string instruction = "<BTCUSDT:kline>{1m=8|/cuwacunu/data/raw/BTCUSDT/1m/BTCUSDT-1m-all-years.csv, 30m=8|/cuwacunu/data/raw/BTCUSDT/30m/BTCUSDT-30m-all-years.csv, 1h=4|/cuwacunu/data/raw/BTCUSDT/1h/BTCUSDT-1h-all-years.csv}";
    // std::string instruction = "<BTCUSDT:kline>{1h=24|/cuwacunu/data/raw/BTCUSDT/1h/BTCUSDT-1h-all-years.csv}";
    // std::string instruction = "<BTCUSDT:kline>{1m=8|/cuwacunu/data/raw/BTCUSDT/1m/BTCUSDT-1m-all-years.csv, 1h=4|/cuwacunu/data/raw/BTCUSDT/1h/BTCUSDT-1h-all-years.csv}";
    std::string instruction = R"(
        observation_setup => [
            {
                step_interval       => 1m
                record_type         => kline
                sequence_length     => 8
                normalization       => {
                    active          => true
                    rolling_window  => 100
                }
                source              => [
                    {
                        source_type => csv
                        instrument  => BTCUSDT
                        files       => [
                            /cuwacunu/data/raw/BTCUSDT/1m/BTCUSDT-1m-all-years.csv
                        ]
                    }
                ]
            }
        ]
    )";
    std::string instruction = R"(
    instrument   |    step_interval   |      Active        |     record_type     |    sequence_length    |    normalization:window    |     source       
---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
     BTCUSDT     |        1m          |       true         |         kline       |            8          |            100             |     /cuwacunu/data/raw/BTCUSDT/1m/BTCUSDT-1m-all-years.csv 
     BTCUSDT     |        1m          |       true         |         kline       |            8          |            100             |     /cuwacunu/data/raw/BTCUSDT/1m/BTCUSDT-1m-all-years.csv 
     BTCUSDT     |        1m          |       true         |         kline       |            8          |            100             |     /cuwacunu/data/raw/BTCUSDT/1m/BTCUSDT-1m-all-years.csv 
     BTCUSDT     |        1m          |       true         |         kline       |            8          |            100             |     /cuwacunu/data/raw/BTCUSDT/1m/BTCUSDT-1m-all-years.csv 
     BTCUSDT     |        1m          |       true         |         kline       |            8          |            100             |     /cuwacunu/data/raw/BTCUSDT/1m/BTCUSDT-1m-all-years.csv 
     BTCUSDT     |        1m          |       true         |         kline       |            8          |            100             |     /cuwacunu/data/raw/BTCUSDT/1m/BTCUSDT-1m-all-years.csv 
     BTCUSDT     |        1m          |       true         |         kline       |            8          |            100             |     /cuwacunu/data/raw/BTCUSDT/1m/BTCUSDT-1m-all-years.csv 
     BTCUSDT     |        1m          |       true         |         kline       |            8          |            100             |     /cuwacunu/data/raw/BTCUSDT/1m/BTCUSDT-1m-all-years.csv 
     BTCUSDT     |        1m          |       true         |         kline       |            8          |            100             |     /cuwacunu/data/raw/BTCUSDT/1m/BTCUSDT-1m-all-years.csv 
     BTCUSDT     |        1m          |       true         |         kline       |            8          |            100             |     /cuwacunu/data/raw/BTCUSDT/1m/BTCUSDT-1m-all-years.csv 
     BTCUSDT     |        1m          |       true         |         kline       |            8          |            100             |     /cuwacunu/data/raw/BTCUSDT/1m/BTCUSDT-1m-all-years.csv 
     BTCUSDT     |        1m          |       true         |         kline       |            8          |            100             |     /cuwacunu/data/raw/BTCUSDT/1m/BTCUSDT-1m-all-years.csv 
     BTCUSDT     |        1m          |       true         |         kline       |            8          |            100             |     /cuwacunu/data/raw/BTCUSDT/1m/BTCUSDT-1m-all-years.csv 
     BTCUSDT     |        1m          |       true         |         kline       |            8          |            100             |     /cuwacunu/data/raw/BTCUSDT/1m/BTCUSDT-1m-all-years.csv 
     BTCUSDT     |        1m          |       true         |         kline       |            8          |            100             |     /cuwacunu/data/raw/BTCUSDT/1m/BTCUSDT-1m-all-years.csv 
     BTCUSDT     |        1m          |       true         |         kline       |            8          |            100             |     /cuwacunu/data/raw/BTCUSDT/1m/BTCUSDT-1m-all-years.csv 
     BTCUSDT     |        1m          |       true         |         kline       |            8          |            100             |     /cuwacunu/data/raw/BTCUSDT/1m/BTCUSDT-1m-all-years.csv 
--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

        observation_setup => [
            {
                step_interval       => 1m
                record_type         => kline
                sequence_length     => 8
                normalization       => {
                    active          => true
                    rolling_window  => 100
                }
                source              => [
                    {
                        source_type => csv
                        instrument  => BTCUSDT
                        files       => [
                            /cuwacunu/data/raw/BTCUSDT/1m/BTCUSDT-1m-all-years.csv
                        ]
                    }
                ]
            }
        ]
    )";

    std::string OBSERVATION_PIPELINE_BNF_GRAMMAR = R"(
<instruction>    ::= "<" <symbol> ":" <data_type> ">" "{" {<sequence_item>} "}" ;
<symbol>         ::= {<letter>} ;
<data_type>      ::= "kline" | "trade" ;
<sequence_item>  ::= <input_form> <delimiter> | <input_form> ;
<input_form>     ::= <interval> "=" <count> "|" <file_path> ;
<file_path>      ::= {<character>};
<character>      ::= <number> | <special> | <letter> ;
<delimiter>      ::= ", " | "," ;
<interval>       ::= "1s" | "1m" | "3m" | "5m" | "15m" | "30m" | "1h" | "2h" | "4h" | "6h" | "8h" | "12h" | "1d" | "3d" | "1w" | "1M" ;
<count>          ::= {<number>} ;
<number>         ::= "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9" ;
<letter>         ::= "A" | "B" | "C" | "D" | "E" | "F" | "G" | "H" | "I" | "J" | "K" | "L" | "M" | "N" | "O" | "P" | "Q" | "R" | "S" | "T" | "U" | "V" | "W" | "X" | "Y" | "Z" | "a" | "b" | "c" | "d" | "e" | "f" | "g" | "h" | "i" | "j" | "k" | "l" | "m" | "n" | "o" | "p" | "q" | "r" | "s" | "t" | "u" | "v" | "w" | "x" | "y" | "z";
<special>        ::= "." | "-" | "_" | "\" | "/";
)";

    size_t buffer_size = 1024;
    char delimiter = ',';
    
    /* create the observation pipeline */
    auto obsPipe = cuwacunu::camahjucunu::BNF::observationPipeline();
    cuwacunu::camahjucunu::BNF::observation_pipeline_instruction_t decoded_data = obsPipe.decode(instruction);

    /* create the dataset for kline_t */
    auto concat_dataset = cuwacunu::camahjucunu::data::MemoryMappedConcatDataset<
        cuwacunu::camahjucunu::exchange::kline_t>();

    /* add the datasets to the concatdataset as per in the instruction */
    for(auto& item: decoded_data.items) {
        auto N = std::stoul(item.count);
        concat_dataset.add_dataset(item.file_path, N /* dataset loads sequences of size N */, N /* normalization window of size N */, true /* force_binarization */, buffer_size, delimiter);
    }

    /* produce the test tensor closest to a specific key value (e.g., close_time) */
    std::vector<int64_t> targets = {
        1578567599999,
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
        auto [features_by_date, mask] = concat_dataset.get_by_key_value(target_close_time);
        PRINT_TOCK_ns(GET_BY_KEY_);
        std::cout << "\t\t\t features_by_date.shape(): "; for (const auto& dim : features_by_date.sizes()) { std::cout << dim << " "; } std::cout << std::endl;
        std::cout << "\t\t\t mask.shape(): "; for (const auto& dim : mask.sizes()) { std::cout << dim << " "; } std::cout << std::endl;
        // std::cout << "\t features_by_date.close_time: " << target_close_time << "=?" << features_by_date.index({0,6}).item<double>() << std::endl;
    }

    log_info("-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- \n");
    log_info("--     get_sequence_ending_at_key_value      -- \n");
    log_info("-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- \n");
    /* --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- */
    /*            test get_sequence_ending_at_key_value            */
    /* --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- */
    
    for(int64_t target_close_time: targets) {
        TICK(GET_SEQUENCE_BY_KEY_1);
        auto [sequence_by_date, mask] = concat_dataset.get_sequence_ending_at_key_value(target_close_time);
        PRINT_TOCK_ns(GET_SEQUENCE_BY_KEY_1);
        std::cout << "\t\t\t sequence_by_date.shape(): "; for (const auto& dim : sequence_by_date.sizes()) { std::cout << dim << " "; } std::cout << std::endl;
        std::cout << "\t\t\t mask.shape(): "; for (const auto& dim : mask.sizes()) { std::cout << dim << " "; } std::cout << std::endl;
        
        std::cout << "--- --- --- SEQUENCE:" << std::endl;
        for (int i = 0; i < sequence_by_date.size(0); ++i) {
            std::cout << "Slice " << i << ":\n";
            for (int j = 0; j < sequence_by_date.size(1); ++j) {
                std::cout << "  Row " << j << ": ";
                for (int k = 0; k < sequence_by_date.size(2); ++k) {
                    float value = sequence_by_date[i][j][k].item<float>();
                    std::cout << std::setw(16) << std::fixed << std::setprecision(4) << value;
                }
                std::cout << '\n';
            }
            std::cout << '\n';
        }

        std::cout << "--- --- --- MASK:" << std::endl;
        for (int i = 0; i < mask.size(0); ++i) {
            std::cout << "Slice " << i << ":\n";
            for (int j = 0; j < mask.size(1); ++j) {
                std::cout << "Row " << j << ": ";
                float value = mask[i][j].item<float>();
                std::cout << std::setw(16) << std::fixed << std::setprecision(4) << value;
                std::cout << '\n';
            }
            std::cout << '\n';
        }
    }

    return 0;
}
