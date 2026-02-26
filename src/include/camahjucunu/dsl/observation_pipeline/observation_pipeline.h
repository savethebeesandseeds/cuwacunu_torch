/* observation_pipeline.h */
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "piaabo/dutils.h"
#include "camahjucunu/types/types_enums.h"

#undef OBSERVARION_PIPELINE_DEBUG /* define to see verbose parsing output */

DEFINE_HASH(OBSERVATION_PIPELINE_HASH_instruction,            "<instruction>");
DEFINE_HASH(OBSERVATION_PIPELINE_HASH_instrument_table,       "<instrument_table>");
DEFINE_HASH(OBSERVATION_PIPELINE_HASH_input_table,            "<input_table>");
DEFINE_HASH(OBSERVATION_PIPELINE_HASH_instrument_header_line, "<instrument_header_line>");
DEFINE_HASH(OBSERVATION_PIPELINE_HASH_instrument_form,        "<instrument_form>");
DEFINE_HASH(OBSERVATION_PIPELINE_HASH_input_header_line,      "<input_header_line>");
DEFINE_HASH(OBSERVATION_PIPELINE_HASH_input_form,             "<input_form>");
DEFINE_HASH(OBSERVATION_PIPELINE_HASH_table_top_line,         "<table_top_line>");
DEFINE_HASH(OBSERVATION_PIPELINE_HASH_table_divider_line,     "<table_divider_line>");
DEFINE_HASH(OBSERVATION_PIPELINE_HASH_table_bottom_line,      "<table_bottom_line>");
DEFINE_HASH(OBSERVATION_PIPELINE_HASH_comment,                "<comment>");
DEFINE_HASH(OBSERVATION_PIPELINE_HASH_norm_window,            "<norm_window>");
DEFINE_HASH(OBSERVATION_PIPELINE_HASH_source,                 "<source>");
DEFINE_HASH(OBSERVATION_PIPELINE_HASH_break_block,            "<break_block>");
DEFINE_HASH(OBSERVATION_PIPELINE_HASH_file_path,              "<file_path>");
DEFINE_HASH(OBSERVATION_PIPELINE_HASH_active,                 "<active>");
DEFINE_HASH(OBSERVATION_PIPELINE_HASH_seq_length,             "<seq_length>");
DEFINE_HASH(OBSERVATION_PIPELINE_HASH_future_seq_length,      "<future_seq_length>");
DEFINE_HASH(OBSERVATION_PIPELINE_HASH_channel_weight,         "<channel_weight>");
DEFINE_HASH(OBSERVATION_PIPELINE_HASH_character,              "<character>");
DEFINE_HASH(OBSERVATION_PIPELINE_HASH_literal,                "<literal>");
DEFINE_HASH(OBSERVATION_PIPELINE_HASH_whitespace,             "<whitespace>");
DEFINE_HASH(OBSERVATION_PIPELINE_HASH_instrument,             "<instrument>");
DEFINE_HASH(OBSERVATION_PIPELINE_HASH_record_type,            "<record_type>");
DEFINE_HASH(OBSERVATION_PIPELINE_HASH_interval,               "<interval>");
DEFINE_HASH(OBSERVATION_PIPELINE_HASH_boolean,                "<boolean>");
DEFINE_HASH(OBSERVATION_PIPELINE_HASH_special,                "<special>");
DEFINE_HASH(OBSERVATION_PIPELINE_HASH_letter,                 "<letter>");
DEFINE_HASH(OBSERVATION_PIPELINE_HASH_number,                 "<number>");
DEFINE_HASH(OBSERVATION_PIPELINE_HASH_newline,                "<newline>");
DEFINE_HASH(OBSERVATION_PIPELINE_HASH_empty,                  "<empty>");
DEFINE_HASH(OBSERVATION_PIPELINE_HASH_frame_char,             "<frame_char>");

namespace cuwacunu {
namespace camahjucunu {

struct source_form_t {
  std::string instrument;
  cuwacunu::camahjucunu::exchange::interval_type_e interval;
  std::string record_type;
  std::string source;
};

struct channel_form_t {
  cuwacunu::camahjucunu::exchange::interval_type_e interval;
  std::string active;
  std::string record_type;
  std::string seq_length;
  std::string future_seq_length;
  std::string channel_weight;
  std::string norm_window;
};

struct observation_instruction_t {
  std::vector<source_form_t> source_forms;
  std::vector<channel_form_t> channel_forms;

  std::vector<source_form_t> filter_source_forms(
    const std::string& target_instrument,
    const std::string& target_record_type,
    cuwacunu::camahjucunu::exchange::interval_type_e target_interval) const;
  std::vector<float> retrieve_channel_weights();
  int64_t count_channels();
  int64_t max_sequence_length();
  int64_t max_future_sequence_length();
};

observation_instruction_t decode_observation_instruction_from_config();
observation_instruction_t decode_observation_instruction_from_split_dsl(
    std::string source_grammar,
    std::string source_instruction,
    std::string channel_grammar,
    std::string channel_instruction);

std::string observation_instruction_source_dump_from_config();

struct observation_pipeline_t {
  static observation_instruction_t inst;
  static void update();

private:
  static void init();
  static void finit();
  struct _init {
    _init()  { observation_pipeline_t::init(); }
    ~_init() { observation_pipeline_t::finit(); }
  };
  static _init _initializer;
};

} /* namespace camahjucunu */
} /* namespace cuwacunu */
