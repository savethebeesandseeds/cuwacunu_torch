/* observation_spec.h */
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

struct observation_source_t {
  std::string instrument;
  cuwacunu::camahjucunu::exchange::interval_type_e interval;
  std::string record_type;
  std::string source;
};

struct observation_channel_t {
  cuwacunu::camahjucunu::exchange::interval_type_e interval;
  std::string active;
  std::string record_type;
  std::string seq_length;
  std::string future_seq_length;
  std::string channel_weight;
  std::string norm_window;
};

struct observation_spec_t {
  std::vector<observation_source_t> source_forms;
  std::vector<observation_channel_t> channel_forms;

  std::vector<observation_source_t> filter_source_forms(
    const std::string& target_instrument,
    const std::string& target_record_type,
    cuwacunu::camahjucunu::exchange::interval_type_e target_interval) const;
  std::vector<float> retrieve_channel_weights();
  int64_t count_channels();
  int64_t max_sequence_length();
  int64_t max_future_sequence_length();
};

observation_spec_t decode_observation_spec_from_contract(
    const std::string& contract_hash);
observation_spec_t decode_observation_spec_from_split_dsl(
    std::string source_grammar,
    std::string source_instruction,
    std::string channel_grammar,
    std::string channel_instruction);

std::string observation_spec_source_dump_from_contract(
    const std::string& contract_hash);

struct observation_runtime_t {
  static observation_spec_t inst;
  static void update(const std::string& contract_hash);

private:
  static void init();
  static void finit();
  struct _init {
    _init()  { observation_runtime_t::init(); }
    ~_init() { observation_runtime_t::finit(); }
  };
  static _init _initializer;
};

} /* namespace camahjucunu */
} /* namespace cuwacunu */
