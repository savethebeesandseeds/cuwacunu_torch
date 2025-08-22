// test_bnf_observation_pipeline.cpp
#include <iostream>
#include "piaabo/dconfig.h"
#include "camahjucunu/BNF/implementations/observation_pipeline/observation_pipeline.h"

int main() {
    
    const char* config_folder = "/cuwacunu/src/config/";
    cuwacunu::piaabo::dconfig::config_space_t::change_config_file(config_folder);
    cuwacunu::piaabo::dconfig::config_space_t::update_config();

    std::string instruction = cuwacunu::piaabo::dconfig::config_space_t::observation_pipeline_instruction();

    TICK(observationPipeline_loadGrammar);
    auto obsPipe = cuwacunu::camahjucunu::BNF::observationPipeline();
    PRINT_TOCK_ns(observationPipeline_loadGrammar);

    TICK(decode_Instruction);
    cuwacunu::camahjucunu::BNF::observation_instruction_t decoded_data = obsPipe.decode(instruction);
    PRINT_TOCK_ns(decode_Instruction);

    log_info("At the end, decoded_data.instrument_forms[size=%ld] \n", decoded_data.instrument_forms.size());
    log_info("\t\t  --- --- --- --- --- \n");
    for(size_t count = 0; count < decoded_data.instrument_forms.size() ; count++) {
        log_info("\t\t  decoded_data.instrument_forms[%ld].instrument  : %s \n", count, decoded_data.instrument_forms[count].instrument.c_str());
        log_info("\t\t  decoded_data.instrument_forms[%ld].interval    : %s \n", count, cuwacunu::camahjucunu::exchange::enum_to_string<cuwacunu::camahjucunu::exchange::interval_type_e>(decoded_data.instrument_forms[count].interval).c_str());
        log_info("\t\t  decoded_data.instrument_forms[%ld].record_type : %s \n", count, decoded_data.instrument_forms[count].record_type.c_str());
        log_info("\t\t  decoded_data.instrument_forms[%ld].norm_window : %s \n", count, decoded_data.instrument_forms[count].norm_window.c_str());
        log_info("\t\t  decoded_data.instrument_forms[%ld].source      : %s \n", count, decoded_data.instrument_forms[count].source.c_str());
        log_info("\t\t  --- --- --- --- --- \n");
    }


    log_info("\t\t  --- --- --- --- --- ... --- --- --- --- --- \n");

    log_info("At the end, decoded_data.input_forms[size=%ld] \n", decoded_data.input_forms.size());
    log_info("\t\t  --- --- --- --- --- \n");
    for(size_t count = 0; count < decoded_data.input_forms.size() ; count++) {
        log_info("\t\t  decoded_data.input_forms[%ld].interval    : %s \n", count, cuwacunu::camahjucunu::exchange::enum_to_string<cuwacunu::camahjucunu::exchange::interval_type_e>(decoded_data.input_forms[count].interval).c_str());
        log_info("\t\t  decoded_data.input_forms[%ld].active      : %s \n", count, decoded_data.input_forms[count].active.c_str());
        log_info("\t\t  decoded_data.input_forms[%ld].record_type : %s \n", count, decoded_data.input_forms[count].record_type.c_str());
        log_info("\t\t  decoded_data.input_forms[%ld].seq_length  : %s \n", count, decoded_data.input_forms[count].seq_length.c_str());
        log_info("\t\t  decoded_data.input_forms[%ld].future_seq_length  : %s \n", count, decoded_data.input_forms[count].future_seq_length.c_str());
        log_info("\t\t  --- --- --- --- --- \n");
    }

    return 0;
}
