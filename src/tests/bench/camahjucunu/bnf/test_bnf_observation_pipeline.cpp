// test_bnf_observation_pipieline.cpp
#include <iostream>
#include "camahjucunu/BNF/implementations/observation_pipeline/observation_pipeline.h"

int main() {
    try {
        std::string instruction = "<BTCUSDT>{1s=60, 1m=60, 1h=24}[path/to/file.csv]";
        
        TICK(observationPipeline_loadGrammar);
        auto obsPipe = cuwacunu::camahjucunu::BNF::observationPipeline();
        PRINT_TOCK_ns(observationPipeline_loadGrammar);

        TICK(decode_Instruction);
        cuwacunu::camahjucunu::BNF::observation_pipeline_instruction_t decoded_data = obsPipe.decode(instruction);
        PRINT_TOCK_ns(decode_Instruction);

        log_info("At the end, decoded_data.symbol: %s \n", decoded_data.symbol.c_str());
        
        log_info("At the end, decoded_data.items[size=%ld] \n", decoded_data.items.size());
        for(size_t count = 0; count < decoded_data.items.size() ; count++) {
            log_info("\t\t  decoded_data.items[%ld] : %s \n", count, cuwacunu::camahjucunu::exchange::enum_to_string<cuwacunu::camahjucunu::exchange::interval_type_e>(decoded_data.items[count].interval).c_str());
            log_info("\t\t  decoded_data.items[%ld] : %ld \n", count, decoded_data.items[count].count);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Parsing error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
