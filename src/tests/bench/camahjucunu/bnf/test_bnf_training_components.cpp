// test_bnf_training_components.cpp
#include <iostream>
#include "piaabo/dconfig.h"
#include "camahjucunu/BNF/implementations/training_components/training_components.h"

int main() {
    
    const char* config_folder = "/cuwacunu/src/config/";
    cuwacunu::piaabo::dconfig::config_space_t::change_config_file(config_folder);
    cuwacunu::piaabo::dconfig::config_space_t::update_config();

    std::string instruction = cuwacunu::piaabo::dconfig::config_space_t::training_components_instruction();

    TICK(trainingPipeline_loadGrammar);
    auto trainPipe = cuwacunu::camahjucunu::BNF::trainingPipeline();
    PRINT_TOCK_ns(trainingPipeline_loadGrammar);

    TICK(decode_Instruction);
    cuwacunu::camahjucunu::BNF::training_instruction_t decoded_data = trainPipe.decode(instruction);
    PRINT_TOCK_ns(decode_Instruction);

    log_info("At the end, decoded_data.raw[size=%ld] \n", decoded_data.raw.size());
    log_info("Tables: \n%s\n", decoded_data.str().c_str());

    return 0;
}
