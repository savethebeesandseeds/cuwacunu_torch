// test_dsl_jkimyei_specs.cpp
#include <iostream>
#include "piaabo/dconfig.h"
#include "camahjucunu/dsl/jkimyei_specs/jkimyei_specs.h"

int main() {
    
    const char* config_folder = "/cuwacunu/src/config/";
    cuwacunu::piaabo::dconfig::config_space_t::change_config_file(config_folder);
    cuwacunu::piaabo::dconfig::config_space_t::update_config();

    std::string instruction = cuwacunu::piaabo::dconfig::contract_space_t::jkimyei_specs_dsl();

    TICK(jkimyeiSpecsPipeline_loadGrammar);
    auto trainPipe = cuwacunu::camahjucunu::dsl::jkimyeiSpecsPipeline();
    PRINT_TOCK_ns(jkimyeiSpecsPipeline_loadGrammar);

    TICK(decode_Instruction);
    cuwacunu::camahjucunu::jkimyei_specs_t decoded_data = trainPipe.decode(instruction);
    PRINT_TOCK_ns(decode_Instruction);

    log_info("At the end, decoded_data.raw[size=%ld] \n", decoded_data.raw.size());
    log_info("Tables: \n%s\n", decoded_data.str().c_str());

    return 0;
}
