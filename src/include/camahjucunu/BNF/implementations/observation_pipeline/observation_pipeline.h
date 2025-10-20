/* observation_pipeline.h */
#pragma once
#include "piaabo/dutils.h"
#include "piaabo/dfiles.h"
#include "piaabo/dconfig.h"
#include "camahjucunu/BNF/BNF_AST.h"
#include "camahjucunu/BNF/BNF_visitor.h"
#include "camahjucunu/BNF/BNF_grammar_lexer.h"
#include "camahjucunu/BNF/BNF_grammar_parser.h"
#include "camahjucunu/BNF/BNF_instruction_lexer.h"
#include "camahjucunu/BNF/BNF_instruction_parser.h"
#include "camahjucunu/types/types_enums.h"

/* 
  Observation Pipeline Grammar:
    Instruction Examples: 
      <BTCUSDT:kline>{1s=60, 1m=60, 1h=24}(path/to/file.csv)
      <BTCUSDT:kline>{1s=15, 1h=5, 1d=10, 1M=2}(path/to/file.csv)
      <BTCUSDT:kline>{1s=60, 1m=5, 5m=3, 15m=2, 30m=2, 1h=24}(path/to/file.csv)
    With this the Pipeline would know to request the Broker or Query de Data 
    For pattern instructed. E.g. <BTCUSDT:kline>{1s=60, 1m=60, 1h=24}(path/to/file.csv) :
      Literal "BTCUSDT" would be parsed to be the symbol
      60 candles of 1 seconds interval  []
      60 candles of 1 minute interval   []
      24 candles of 1 hour interval     []
*/

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

struct instrument_form_t {
  std::string instrument;
  cuwacunu::camahjucunu::exchange::interval_type_e interval;
  std::string record_type;
  std::string norm_window;
  std::string source;
};

struct input_form_t {
  cuwacunu::camahjucunu::exchange::interval_type_e interval;
  std::string active;
  std::string record_type;
  std::string seq_length;
  std::string future_seq_length;
  std::string channel_weight;
};

struct observation_instruction_t {
  std::vector<instrument_form_t> instrument_forms;
  std::vector<input_form_t> input_forms;
  std::vector<instrument_form_t> filter_instrument_forms(
    const std::string& target_instrument,
    const std::string& target_record_type,
    cuwacunu::camahjucunu::exchange::interval_type_e target_interval) const;
  std::vector<float> retrieve_channel_weights();
  int64_t count_channels();
  int64_t max_sequence_length();
  int64_t max_future_sequence_length();
};

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

/* 
 * observationPipeline is a concrete visitor that traverses the AST to 
 * extract execution data (symbol, parameters, file IDs) and executes 
 * corresponding functions based on the parsed instructions.
 */
namespace BNF {
class observationPipeline : public ASTVisitor {
private:
  std::mutex current_mutex;

public:

  std::string OBSERVATION_PIPELINE_BNF_GRAMMAR = cuwacunu::piaabo::dconfig::config_space_t::observation_pipeline_bnf();

  GrammarLexer bnfLexer;
  GrammarParser bnfParser;
  ProductionGrammar grammar;
  InstructionLexer iLexer;
  InstructionParser iParser;  
  
  /* Constructor: Registers executable functions */
  observationPipeline();

  /* Decode: Interprest an instruction string */
  observation_instruction_t decode(std::string instruction);

  /* parse Grammar (dummy) */
  ProductionGrammar parseBnfGrammar();

  /* Override visit methods for each AST node type */
  void visit(const RootNode* node, VisitorContext& context) override;
  void visit(const IntermediaryNode* node, VisitorContext& context) override;
  void visit(const TerminalNode* node, VisitorContext& context) override;
};
} /* namespace BNF */

} /* namespace camahjucunu */
} /* namespace cuwacunu */
